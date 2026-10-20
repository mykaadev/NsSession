// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Misc/AutomationTest.h"
#include "NsSessionSubsystem.h"
#include "NsSessionTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNsSessionRoomCodeTest, "NsSession.RoomCode",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNsSessionRoomCodeTest::RunTest(const FString& Parameters)
{
    // A new code must pass the same check that a typed code passes
    for (int32 Index = 0; Index < 50; ++Index)
    {
        const FString Code = UNsSessionSubsystem::MakeRoomCode();
        TestEqual(TEXT("A new code is a valid code"), UNsSessionSubsystem::NormalizeRoomCode(Code), Code);
    }
    TestEqual(TEXT("A typed code goes to upper case"), UNsSessionSubsystem::NormalizeRoomCode(TEXT(" kxqtp ")), FString(TEXT("KXQTP")));
    TestTrue(TEXT("A short code is refused"), UNsSessionSubsystem::NormalizeRoomCode(TEXT("KXQT")).IsEmpty());
    TestTrue(TEXT("The letter O is refused"), UNsSessionSubsystem::NormalizeRoomCode(TEXT("KXQTO")).IsEmpty());
    TestTrue(TEXT("A digit is refused"), UNsSessionSubsystem::NormalizeRoomCode(TEXT("KXQT1")).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNsSessionStatusTextTest, "NsSession.StatusText",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNsSessionStatusTextTest::RunTest(const FString& Parameters)
{
    // Every status has a default line, so a game that shows the default text never shows an empty line
    const UEnum* const StatusEnum = StaticEnum<ENsRoomStatus>();
    for (int32 Index = 0; Index < StatusEnum->NumEnums() - 1; ++Index)
    {
        FNsRoomStatus Status;
        Status.Status = static_cast<ENsRoomStatus>(StatusEnum->GetValueByIndex(Index));
        Status.RoomCode = TEXT("KXQTP");
        TestFalse(FString::Printf(TEXT("%s has a default line"), *StatusEnum->GetNameStringByIndex(Index)), Status.GetDefaultText().IsEmpty());
    }
    return true;
}

#endif
