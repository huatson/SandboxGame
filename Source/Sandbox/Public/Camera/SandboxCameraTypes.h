#pragma once

#include "CoreMinimal.h"
#include "SandboxCameraTypes.generated.h"

/*-----------------------------------------------------------------------------
Helpers
-----------------------------------------------------------------------------*/

static FName CAMERA_THIRDPERSON(TEXT("Default"));
static FName CAMERA_SCREENSHOT(TEXT("Screenshot"));
static FName CAMERA_FREE(TEXT("FreeCam"));
static const float Rad2Deg = 180.f / PI;
static const float Deg2Rad = PI / 180.f;

static FRotator RotatorLerp(FRotator const& A, FRotator const& B, float Alpha)
{
	FRotator const DeltaAngle = (B - A).GetNormalized();
	return A + (Alpha * DeltaAngle);
}

static inline FVector TransformWorldToLocal(FVector const& WorldVect, FRotator const& SystemRot)
{
	return FRotationMatrix(SystemRot).InverseTransformVector(WorldVect);
}

static inline FVector TransformLocalToWorld(FVector const& LocalVect, FRotator const& SystemRot)
{
	return FRotationMatrix(SystemRot).TransformVector(LocalVect);
}

static float GetHeadingAngle(FVector const& Dir)
{
	float Angle = FGenericPlatformMath::Acos(FMath::Clamp<float>(Dir.X, -1.f, 1.f));
	if (Dir.Y < 0.f)
	{
		Angle *= -1.f;
	}

	return Angle;
}

static inline float DegToUnrRotatorUnits(float InDeg)
{
	return InDeg * 182.0444f;
}

static FRotator RInterpToWithPerAxisSpeeds(FRotator const& Current, FRotator const& Target, float DeltaTime, float PitchInterpSpeed, float YawInterpSpeed, float RollInterpSpeed)
{
	if (DeltaTime == 0.f || Current == Target)
	{
		return Current;
	}

	FRotator DeltaMove = (Target - Current).GetNormalized();
	DeltaMove.Pitch = DeltaMove.Pitch * FMath::Clamp<float>(DeltaTime * PitchInterpSpeed, 0.f, 1.f);
	DeltaMove.Yaw = DeltaMove.Yaw * FMath::Clamp<float>(DeltaTime * YawInterpSpeed, 0.f, 1.f);
	DeltaMove.Roll = DeltaMove.Roll * FMath::Clamp<float>(DeltaTime * RollInterpSpeed, 0.f, 1.f);
	return (Current + DeltaMove).GetNormalized();
}

static inline float RadToUnrRotatorUnits(float InDeg)
{
	return InDeg * 180.f / PI * 182.0444f;
}

static inline float FPctByRange(float Value, float InMin, float InMax)
{
	return (Value - InMin) / (InMax - InMin);
}



USTRUCT()
struct FPenetrationAvoidanceFeeler
{
	GENERATED_USTRUCT_BODY()

	FRotator AdjustmentRot;
	float WorldWeight;
	float PawnWeight;
	FVector Extent;
	int32 TraceInterval;
	int32 FramesUntilNextTrace;
	FPenetrationAvoidanceFeeler()
		: AdjustmentRot(FRotator::ZeroRotator)
		, WorldWeight(0.f)
		, PawnWeight(0.f)
		, Extent(FVector::ZeroVector)
		, TraceInterval(INDEX_NONE)
		, FramesUntilNextTrace(INDEX_NONE)
	{
		//
	}
};


UENUM()
enum ECameraViewportTypes
{
	CVT_16to9_Full,
	CVT_16to9_VertSplit,
	CVT_16to9_HorizSplit,
	CVT_4to3_Full,
	CVT_4to3_HorizSplit,
	CVT_4to3_VertSplit
};


UENUM()
enum ECameraAnimOption
{
	CAO_None					UMETA(DisplayName = "None"),
	CAO_TranslateDelta			UMETA(DisplayName = "TranslateDelta"),
	CAO_TranslateRotateDelta	UMETA(DisplayName = "TranslateRotateDelta"),
	CAO_Absolute				UMETA(DisplayName = "Absolute")
};



USTRUCT(BlueprintType)
struct FViewOffsetData
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, Category = "ViewOffsetData")
	FVector OffsetHigh;
	UPROPERTY(EditAnywhere, Category = "ViewOffsetData")
	FVector OffsetMid;
	UPROPERTY(EditAnywhere, Category = "ViewOffsetData")
	FVector OffsetLow;
	FViewOffsetData()
		: OffsetHigh(FVector::ZeroVector)
		, OffsetMid(FVector::ZeroVector)
		, OffsetLow(FVector::ZeroVector)
	{
		//
	}
};