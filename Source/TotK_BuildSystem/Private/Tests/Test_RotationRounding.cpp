// Fill out your copyright notice in the Description page of Project Settings.

// To run tests, enter console command - Automation RunTests GrabSystem.RotationRounding

#include "Tests/Test_RotationRounding.h"
#include "Misc/AutomationTest.h"
#include "Math/Quat.h"
#include "Math/Rotator.h"
#include "Math/UnrealMathUtility.h"

// Round off the specified pitch, yaw, or roll value
float CalculateRotation(float CurrRot, float CurrMod)
{
	float RotationDegrees = 45;
	float RetRot = 0.0f;
	float HalfRot = RotationDegrees / 2;

	// Calculate the rounded values based on the mod values for pitch, yaw, and roll. For example: With RotationDegrees set to 45:
	// If the currMod is close to 0, no adjustment is needed
	if (FMath::Abs(CurrMod) < 0.01f) {
		RetRot = 0;
	}

	// Calculate the rounded values based on the mod values for pitch, yaw, and roll. For example: With RotationDegrees set to 45:
	// If the current mod is less than -22.5, we want to calculate (held rotation value + (-RotationDegrees - current mod value)) -> (-80 + (-45 - (-80 mod 45 = -35)) = -90)
	else if (CurrMod < -HalfRot) {
		RetRot = CurrRot + (-RotationDegrees - CurrMod);
	}

	// If the current mod is between -22.5 and 0, we want to calculate (held rotation value - current mod value) -> (-147 - (-147 mod 45 = -12) = -135)
	// If the current mod is between 0 and 22.5, we want to calculate (held rotation value - current mod value) -> (94 - (94 mod 45 = 4) = 90)
	else if (CurrMod > -HalfRot && CurrMod < HalfRot) {
		RetRot = CurrRot - CurrMod;
	}

	// Otherwise, the current mod must be greater than 22.5, we want to calculate (held rotation value + (RotationDegrees - current mod value)) -> (68 + (45 - (68 mod 45 = 23)) = 90)
	else {
		RetRot = CurrRot + (RotationDegrees - CurrMod);
	}

	return RetRot;
}

// Round off the grabbed object's initial rotation based on the preferred rotation degrees
static FRotator RoundObjectRotation(const FRotator& Rot)
{
	FRotator ResRotation;
	float RotationDegrees = 45;

	// Get all mods for calculations
	float PitchMod = FMath::Fmod(Rot.Pitch, RotationDegrees);
	float YawMod = FMath::Fmod(Rot.Yaw, RotationDegrees);
	float RollMod = FMath::Fmod(Rot.Roll, RotationDegrees);

	// Calculate the rounded rotations
	ResRotation.Pitch = CalculateRotation(Rot.Pitch, PitchMod);
	ResRotation.Yaw = CalculateRotation(Rot.Yaw, YawMod);
	ResRotation.Roll = CalculateRotation(Rot.Roll, RollMod);

	return ResRotation;
}

// Register the test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRotationRoundingTest,
	"GrabSystem.RotationRounding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FRotationRoundingTest::RunTest(const FString& Parameters)
{
	// Test 1: Small positive offset
	{
		FRotator Input(0.f, 0.0002f, 0.f);
		FRotator Expected(0.f, 0.f, 0.f);
		TestEqual(TEXT("Rounding down from 0.0002°"), RoundObjectRotation(Input), Expected);
	}

	// Test 2: Small positive offset
	{
		FRotator Input(0.f, 22.4f, 0.f);
		FRotator Expected(0.f, 0.f, 0.f);
		TestEqual(TEXT("Rounding down from 22.4°"), RoundObjectRotation(Input), Expected);
	}

	// Test 3: Larger offset (should round up)
	{
		FRotator Input(0.f, 23.6f, 0.f);
		FRotator Expected(0.f, 45.f, 0.f);
		TestEqual(TEXT("Rounding up from 23.6°"), RoundObjectRotation(Input), Expected);
	}

	// Test 4: Negative rounding
	{
		FRotator Input(0.f, -67.f, 0.f);
		FRotator Expected(0.f, -45.f, 0.f);
		TestEqual(TEXT("Rounding up from -67°"), RoundObjectRotation(Input), Expected);
	}

	// Test 5: Code example 1
	{
		FRotator Input(0.f, -80.f, 0.f);
		FRotator Expected(0.f, -90.f, 0.f);
		TestEqual(TEXT("Rounding down from -80°"), RoundObjectRotation(Input), Expected);
	}

	// Test 6: Code example 2
	{
		FRotator Input(0.f, -147.f, 0.f);
		FRotator Expected(0.f, -135.f, 0.f);
		TestEqual(TEXT("Rounding up from -174°"), RoundObjectRotation(Input), Expected);
	}

	// Test 7: Code example 3
	{
		FRotator Input(0.f, 94.f, 0.f);
		FRotator Expected(0.f, 90.f, 0.f);
		TestEqual(TEXT("Rounding down from 90°"), RoundObjectRotation(Input), Expected);
	}

	// Test 8: Code example 4
	{
		FRotator Input(0.f, 68.f, 0.f);
		FRotator Expected(0.f, 90.f, 0.f);
		TestEqual(TEXT("Rounding up from 90°"), RoundObjectRotation(Input), Expected);
	}

	return true;
}