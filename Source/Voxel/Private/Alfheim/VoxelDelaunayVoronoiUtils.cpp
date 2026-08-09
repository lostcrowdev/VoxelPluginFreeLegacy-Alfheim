// VoxelDelaunayVoronoiUtils.cpp

#include "Alfheim/VoxelDelaunayVoronoiUtils.h"

TArray<FVoxelDelaunayVoronoiUtils::FDelaunayTriangle>
FVoxelDelaunayVoronoiUtils::BowyerWatsonFromPoints(const TArray<FVector2D>& Points)
{
	TArray<FDelaunayTriangle> Tri;
	if (Points.Num() < 3) return Tri;

	float MinX=FLT_MAX, MinY=FLT_MAX, MaxX=-FLT_MAX, MaxY=-FLT_MAX;
	for (const FVector2D& P : Points)
	{ MinX=FMath::Min(MinX,P.X); MinY=FMath::Min(MinY,P.Y); MaxX=FMath::Max(MaxX,P.X); MaxY=FMath::Max(MaxY,P.Y); }

	const float DMax = FMath::Max(MaxX-MinX, MaxY-MinY) * 10.f;
	const float MidX = (MinX+MaxX)*.5f, MidY = (MinY+MaxY)*.5f;
	const int32 SA   = Points.Num(), SB=SA+1, SC=SA+2;

	TArray<FVector2D> Pts = Points;
	Pts.Add(FVector2D(MidX-DMax, MidY-DMax));
	Pts.Add(FVector2D(MidX,      MidY+DMax));
	Pts.Add(FVector2D(MidX+DMax, MidY-DMax));

	Tri.Add({SA, SB, SC});

	for (int32 Pi = 0; Pi < Points.Num(); ++Pi)
	{
		const FVector2D& P = Pts[Pi];
		TArray<FDelaunayTriangle> Bad;
		for (const FDelaunayTriangle& T : Tri)
		{ FVector2D CC; float RR; GetCircumcircle(Pts[T.A],Pts[T.B],Pts[T.C],CC,RR); if(FVector2D::DistSquared(CC,P)<=RR) Bad.Add(T); }

		TArray<FDelaunayEdge> Hole;
		for (const FDelaunayTriangle& B : Bad)
		{
			FDelaunayEdge E3[3] = {{B.A,B.B},{B.B,B.C},{B.A,B.C}};
			for (const FDelaunayEdge& E : E3)
			{
				bool bS = false;
				for (const FDelaunayTriangle& O : Bad)
				{ if (&O==&B) continue; if ((O.A==E.A||O.B==E.A||O.C==E.A)&&(O.A==E.B||O.B==E.B||O.C==E.B)){bS=true;break;} }
				if (!bS) Hole.Add(E);
			}
		}
		Tri.RemoveAll([&](const FDelaunayTriangle& T){ for (const FDelaunayTriangle& B:Bad) if(B.A==T.A&&B.B==T.B&&B.C==T.C) return true; return false; });
		for (const FDelaunayEdge& E : Hole) Tri.Add({E.A, E.B, Pi});
	}
	Tri.RemoveAll([&](const FDelaunayTriangle& T){ return T.A>=SA||T.B>=SA||T.C>=SA; });
	return Tri;
}

void FVoxelDelaunayVoronoiUtils::GetCircumcircle(
	const FVector2D& A, const FVector2D& B, const FVector2D& C, FVector2D& OC, float& OR)
{
	const float D = 2.f*(A.X*(B.Y-C.Y)+B.X*(C.Y-A.Y)+C.X*(A.Y-B.Y));
	if (FMath::IsNearlyZero(D)) { OC=(A+B+C)/3.f; OR=FLT_MAX; return; }
	const float A2=A.X*A.X+A.Y*A.Y, B2=B.X*B.X+B.Y*B.Y, C2=C.X*C.X+C.Y*C.Y;
	OC.X=(A2*(B.Y-C.Y)+B2*(C.Y-A.Y)+C2*(A.Y-B.Y))/D;
	OC.Y=(A2*(C.X-B.X)+B2*(A.X-C.X)+C2*(B.X-A.X))/D;
	OR=FVector2D::DistSquared(OC, A);
}

TArray<FVector2D> FVoxelDelaunayVoronoiUtils::ComputeVoronoiPolygon2D(
	int32 Idx, const TArray<FVector2D>& Positions, const TArray<FDelaunayTriangle>& Triangles, const FBox2D& Bounds)
{
	if (!Positions.IsValidIndex(Idx)) return {};
	const FVector2D Center = Positions[Idx];
	TArray<TPair<float,FVector2D>> APs;
	for (const FDelaunayTriangle& T : Triangles)
	{
		if (T.A!=Idx&&T.B!=Idx&&T.C!=Idx) continue;
		if (!Positions.IsValidIndex(T.A)||!Positions.IsValidIndex(T.B)||!Positions.IsValidIndex(T.C)) continue;
		FVector2D CC; float RR;
		GetCircumcircle(Positions[T.A], Positions[T.B], Positions[T.C], CC, RR);
		CC.X=FMath::Clamp(CC.X,Bounds.Min.X,Bounds.Max.X); CC.Y=FMath::Clamp(CC.Y,Bounds.Min.Y,Bounds.Max.Y);
		APs.Add({FMath::Atan2(CC.Y-Center.Y, CC.X-Center.X), CC});
	}
	APs.Sort([](const TPair<float,FVector2D>& A, const TPair<float,FVector2D>& BB){ return A.Key<BB.Key; });
	TArray<FVector2D> R; for (auto& AP : APs) R.Add(AP.Value); return R;
}

TArray<TArray<FVector2D>> FVoxelDelaunayVoronoiUtils::ExtractMarchingSquaresLoops(
	int32 Res, const FBox2D& Bounds,
	TFunctionRef<bool(int32,int32)> InsideFn, TFunctionRef<float(int32,int32)> ValueFn,
	float Threshold, int32 MinLoopSegments)
{
	const float SX=(Bounds.Max.X-Bounds.Min.X)/(Res-1), SY=(Bounds.Max.Y-Bounds.Min.Y)/(Res-1);
	auto GP=[&](int32 R,int32 C)->FVector2D{ return FVector2D(Bounds.Min.X+C*SX, Bounds.Min.Y+R*SY); };
	auto Interp=[&](int32 R0,int32 C0,int32 R1,int32 C1)->FVector2D{
		const float V0=ValueFn(R0,C0), V1=ValueFn(R1,C1); float T=0.5f;
		const float D=V0-V1; if(!FMath::IsNearlyZero(D)) T=FMath::Clamp((V0-Threshold)/D,0.f,1.f);
		return FMath::Lerp(GP(R0,C0), GP(R1,C1), T); };
	struct FSeg { FVector2D P0, P1; };
	TArray<FSeg> Segs; Segs.Reserve((Res-1)*(Res-1));
	for (int32 Row=0; Row<Res-1; ++Row) for (int32 Col=0; Col<Res-1; ++Col)
	{
		const bool TL=InsideFn(Row,Col), TR=InsideFn(Row,Col+1), BR=InsideFn(Row+1,Col+1), BL=InsideFn(Row+1,Col);
		const int32 Case=(TL?1:0)|(TR?2:0)|(BR?4:0)|(BL?8:0);
		if (Case==0||Case==15) continue;
		auto Top=[&]{return Interp(Row,Col,Row,Col+1);}; auto Right=[&]{return Interp(Row,Col+1,Row+1,Col+1);};
		auto Bottom=[&]{return Interp(Row+1,Col,Row+1,Col+1);}; auto Left=[&]{return Interp(Row,Col,Row+1,Col);};
		switch(Case){
		case 1: Segs.Add({Top(),Left()});break; case 2: Segs.Add({Right(),Top()});break;
		case 4: Segs.Add({Bottom(),Right()});break; case 8: Segs.Add({Left(),Bottom()});break;
		case 14:Segs.Add({Top(),Left()});break; case 13:Segs.Add({Right(),Top()});break;
		case 11:Segs.Add({Bottom(),Right()});break; case 7: Segs.Add({Left(),Bottom()});break;
		case 3: Segs.Add({Right(),Left()});break; case 6: Segs.Add({Bottom(),Top()});break;
		case 12:Segs.Add({Right(),Left()});break; case 9: Segs.Add({Bottom(),Top()});break;
		case 5: Segs.Add({Top(),Right()}); Segs.Add({Bottom(),Left()});break;
		case 10:Segs.Add({Right(),Bottom()}); Segs.Add({Left(),Top()});break; default:break; }
	}
	if (Segs.IsEmpty()) return {};
	auto PosKey=[](const FVector2D& P)->uint64
	{ return ((uint64)(uint32)FMath::RoundToInt(P.X)<<32)|(uint64)(uint32)FMath::RoundToInt(P.Y); };
	TMap<uint64,TArray<TPair<int32,int32>>> PMap; PMap.Reserve(Segs.Num()*2);
	for (int32 Si=0; Si<Segs.Num(); ++Si) { PMap.FindOrAdd(PosKey(Segs[Si].P0)).Add({Si,0}); PMap.FindOrAdd(PosKey(Segs[Si].P1)).Add({Si,1}); }
	TArray<bool> Used; Used.Init(false, Segs.Num());
	TArray<TArray<FVector2D>> Loops;
	for (int32 SS=0; SS<Segs.Num(); ++SS)
	{
		if (Used[SS]) continue;
		TArray<FVector2D> Loop; Loop.Add(Segs[SS].P0); Used[SS]=true;
		FVector2D Cur=Segs[SS].P1; const uint64 SK=PosKey(Segs[SS].P0); bool bClosed=false;
		for (int32 It=0; It<Segs.Num(); ++It)
		{
			const uint64 CK=PosKey(Cur); if(CK==SK&&Loop.Num()>2){bClosed=true;break;}
			const auto* Nb=PMap.Find(CK); if(!Nb) break; bool bF=false;
			for (const TPair<int32,int32>& NP:*Nb){ if(Used[NP.Key]) continue; Used[NP.Key]=true; Loop.Add(Cur); Cur=(NP.Value==0)?Segs[NP.Key].P1:Segs[NP.Key].P0; bF=true; break; }
			if (!bF) break;
		}
		if (bClosed && Loop.Num() >= MinLoopSegments) Loops.Add(MoveTemp(Loop));
	}
	return Loops;
}
