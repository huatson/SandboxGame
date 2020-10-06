#pragma once

#include "CoreMinimal.h"
#include "SandboxPawnTypes.generated.h"

class UDamageType;
class FArchive;


USTRUCT(BlueprintType)
struct FCringeInfo
{
	GENERATED_USTRUCT_BODY()

	UDamageType* DamageType;
	TArray<float>		History;
	float				NextCringeTime;
};

UENUM(BlueprintType)
enum ECoverType
{
	CT_None,
	CT_Standing,
	CT_MidLevel
};


UENUM(BlueprintType)
enum ECoverAction
{
	CA_Default,
	CA_BlindLeft,
	CA_BlindRight,
	CA_LeanLeft,
	CA_LeanRight,
	CA_PopUp,
	CA_BlindUp,
	CA_PeekLeft,
	CA_PeekRight,
	CA_PeekUp,
};


UENUM(BlueprintType)
enum ECoverDirection
{
	CD_Default,
	CD_Left,
	CD_Right,
	CD_Up,
};



/**
* Alpha Blend Type
*/
UENUM()
enum EAlphaBlendType
{
	ABT_Linear,					// Linear interpolation
	ABT_Cubic,					// Cubic-in interpolation
	ABT_HermiteCubic,			// Hermite-Cubic
	ABT_SinusoidalInOut,		// Sinusoidal in/out interpolation
	ABT_QuadraticInOut,			// Quadratic in-out interpolation
	ABT_CubicInOut,				// Cubic in-out interpolation
	ABT_QuarticInOut,			// Quartic in-out interpolation
	ABT_QuinticInOut,			// Quintic in-out interpolation
	ABT_CircularIn,				// Circular-in interpolation
	ABT_CircularOut,			// Circular-out interpolation
	ABT_CircularInOut,			// Circular in-out interpolation
	ABT_ExpIn,					// Exponential-in interpolation
	ABT_ExpOut,					// Exponential-Out interpolation
	ABT_ExpInOut				// Exponential in-out interpolation
};

/** Turn a linear interpolated alpha into the corresponding EAlphaBlendType */
FORCEINLINE float CustomAlphaToBlendType(float InAlpha, TEnumAsByte<EAlphaBlendType> BlendType)
{
	switch (BlendType)
	{
	case EAlphaBlendType::ABT_SinusoidalInOut:		return FMath::Clamp<float>((FMath::InterpSinInOut(0.f, (PI*HALF_PI), InAlpha) + 1.f) / 2.f, 0.f, 1.f);
	case EAlphaBlendType::ABT_Cubic:				return FMath::Clamp<float>(FMath::CubicInterp<float>(0.f, 0.f, 1.f, 0.f, InAlpha), 0.f, 1.f);
	case EAlphaBlendType::ABT_QuadraticInOut:		return FMath::Clamp<float>(FMath::InterpEaseInOut<float>(0.f, 1.f, InAlpha, 2), 0.f, 1.f);
	case EAlphaBlendType::ABT_CubicInOut:			return FMath::Clamp<float>(FMath::InterpEaseInOut<float>(0.f, 1.f, InAlpha, 3), 0.f, 1.f);
	case EAlphaBlendType::ABT_HermiteCubic:			return FMath::Clamp<float>(FMath::SmoothStep(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
	case EAlphaBlendType::ABT_QuarticInOut:			return FMath::Clamp<float>(FMath::InterpEaseInOut<float>(0.f, 1.f, InAlpha, 4), 0.f, 1.f);
	case EAlphaBlendType::ABT_QuinticInOut:			return FMath::Clamp<float>(FMath::InterpEaseInOut<float>(0.f, 1.f, InAlpha, 5), 0.f, 1.f);
	case EAlphaBlendType::ABT_CircularIn:			return FMath::Clamp<float>(FMath::InterpCircularIn<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
	case EAlphaBlendType::ABT_CircularOut:			return FMath::Clamp<float>(FMath::InterpCircularOut<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
	case EAlphaBlendType::ABT_CircularInOut:		return FMath::Clamp<float>(FMath::InterpCircularInOut<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
	case EAlphaBlendType::ABT_ExpIn:				return FMath::Clamp<float>(FMath::InterpExpoIn<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
	case EAlphaBlendType::ABT_ExpOut:				return FMath::Clamp<float>(FMath::InterpExpoOut<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
	case EAlphaBlendType::ABT_ExpInOut:				return FMath::Clamp<float>(FMath::InterpExpoInOut<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
	}
	return FMath::Clamp<float>(InAlpha, 0.f, 1.f);
}

USTRUCT()
struct FTAlphaBlend
{
	GENERATED_USTRUCT_BODY()

public:

	/** Internal Lerp value for Alpha */
	float			AlphaIn;
	/** Resulting Alpha value, between 0.f and 1.f */
	float			AlphaOut;
	/** Target to reach */
	float			AlphaTarget;
	/** Default blend time */
	float			BlendTime;
	/** Time left to reach target */
	float			BlendTimeToGo;
	/** Type of blending used (Linear, Cubic, etc.) */
	TEnumAsByte<EAlphaBlendType> BlendType;

	//	Empty Constructor
	FTAlphaBlend() {}

	//	Default Constructor
	FTAlphaBlend(float InAlphaIn, float InAlphaOut, float InAlphaTarget, float InBlendTime, float InBlendTimeToGo, TEnumAsByte<EAlphaBlendType> InBlendType)
		: AlphaIn(InAlphaIn),
		AlphaOut(InAlphaOut),
		AlphaTarget(InAlphaTarget),
		BlendTime(InBlendTime),
		BlendTimeToGo(InBlendTimeToGo),
		BlendType(InBlendType)
	{
		//	empty
	}

	// Serializer.
	friend FArchive& operator<<(FArchive& Ar, FTAlphaBlend& AlphaBlend)
	{
		return Ar << AlphaBlend.AlphaIn << AlphaBlend.AlphaOut << AlphaBlend.AlphaTarget << AlphaBlend.BlendTime << AlphaBlend.BlendTimeToGo << AlphaBlend.BlendType;
	}

	/** Update transition blend time */
	FORCEINLINE void SetBlendTime(float InBlendTime)
	{
		BlendTime = FGenericPlatformMath::Max<float>(InBlendTime, 0.f);
	}

	/** Reset to zero */
	void Reset()
	{
		AlphaIn = 0.f;
		AlphaOut = 0.f;
		AlphaTarget = 0.f;
		BlendTimeToGo = 0.f;
	}

	/** Returns TRUE if Target is > 0.f, or FALSE otherwise */
	FORCEINLINE bool GetToggleStatus()
	{
		return (AlphaTarget > 0.f);
	}

	/** Enable (1.f) or Disable (0.f) */
	FORCEINLINE void Toggle(bool bEnable)
	{
		ConditionalSetTarget(bEnable ? 1.f : 0.f);
	}

	/** SetTarget, but check that we're actually setting a different target */
	FORCEINLINE void ConditionalSetTarget(float InAlphaTarget)
	{
		if (AlphaTarget != InAlphaTarget)
		{
			SetTarget(InAlphaTarget);
		}
	}

	/** Set Target for interpolation */
	void SetTarget(float InAlphaTarget)
	{
		// Clamp parameters to valid range
		AlphaTarget = FMath::Clamp<float>(InAlphaTarget, 0.f, 1.f);

		// if blend time is zero, transition now, don't wait to call update.
		if (BlendTime <= 0.f)
		{
			AlphaIn = AlphaTarget;
			AlphaOut = CustomAlphaToBlendType(AlphaIn, BlendType);
			BlendTimeToGo = 0.f;
		}
		else
		{
			// Blend time is to go all the way, so scale that by how much we have to travel
			BlendTimeToGo = BlendTime * FGenericPlatformMath::Abs(AlphaTarget - AlphaIn);
		}
	}

	/** Update interpolation, has to be called once every frame */
	void Update(float InDeltaTime)
	{
		// Make sure passed in delta time is positive
		check(InDeltaTime >= 0.f);

		if (AlphaIn != AlphaTarget)
		{
			if (BlendTimeToGo >= InDeltaTime)
			{
				const float BlendDelta = AlphaTarget - AlphaIn;
				AlphaIn += (BlendDelta / BlendTimeToGo) * InDeltaTime;
				BlendTimeToGo -= InDeltaTime;
			}
			else
			{
				BlendTimeToGo = 0.f;
				AlphaIn = AlphaTarget;
			}

			// Convert Lerp alpha to corresponding blend type.
			AlphaOut = CustomAlphaToBlendType(AlphaIn, BlendType);
		}
	}
};