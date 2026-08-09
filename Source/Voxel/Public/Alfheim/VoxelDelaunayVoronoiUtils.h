#pragma once

#include "CoreMinimal.h"

struct FVoxelDelaunayVoronoiUtils
{
	struct FDelaunayTriangle
	{
		int32 A = INDEX_NONE;
		int32 B = INDEX_NONE;
		int32 C = INDEX_NONE;
	};

	struct FDelaunayEdge
	{
		int32 A = INDEX_NONE;
		int32 B = INDEX_NONE;

		bool operator==(const FDelaunayEdge& Other) const
		{
			return (A == Other.A && B == Other.B) || (A == Other.B && B == Other.A);
		}
	};

	/** Bowyer-Watson triangulation over an arbitrary 2D point set. */
	static TArray<FDelaunayTriangle> BowyerWatsonFromPoints(const TArray<FVector2D>& Points);

	/** Circumcircle of triangle ABC. OutRadiusSq is the squared circumradius. */
	static void GetCircumcircle(
		const FVector2D& A, const FVector2D& B, const FVector2D& C,
		FVector2D& OutCenter, float& OutRadiusSq);

	/**
	 * Voronoi cell polygon (fan of circumcenters, clipped to Bounds and angle-sorted)
	 * for the point at Positions[Idx], derived from a pre-built Delaunay triangulation
	 * over the same Positions array.
	 */
	static TArray<FVector2D> ComputeVoronoiPolygon2D(
		int32 Idx,
		const TArray<FVector2D>& Positions,
		const TArray<FDelaunayTriangle>& Triangles,
		const FBox2D& Bounds);

	/**
	 * Marching-squares contour extraction over an implicit Res x Res grid spanning
	 * Bounds. InsideFn/ValueFn are sampled at grid coordinates (Row, Col).
	 * Returns closed loops with at least MinLoopSegments points.
	 */
	static TArray<TArray<FVector2D>> ExtractMarchingSquaresLoops(
		int32 Res,
		const FBox2D& Bounds,
		TFunctionRef<bool(int32, int32)> InsideFn,
		TFunctionRef<float(int32, int32)> ValueFn,
		float Threshold,
		int32 MinLoopSegments);
};
