/**
 * Joystick test suite
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_test.h>
#include "../src/joystick/usb_ids.h"
#include "testautomation_suites.h"

/* ================= Test Case Implementation ================== */

/* Fixture */

/* Create a 32-bit writable surface for blitting tests */
static void SDLCALL joystickSetUp(void **arg)
{
    SDL_InitSubSystem(SDL_INIT_GAMEPAD);

    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
}

static void SDLCALL joystickTearDown(void *arg)
{
    SDL_ResetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS);

    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}


/* Test case functions */

/**
 * Check virtual joystick creation
 *
 * \sa SDL_AttachVirtualJoystick
 */
static int SDLCALL joystick_testVirtual(void *arg)
{
    SDL_VirtualJoystickDesc desc;
    SDL_Joystick *joystick = NULL;
    SDL_Gamepad *gamepad = NULL;
    SDL_JoystickID device_id;

    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.vendor_id = USB_VENDOR_NVIDIA;
    desc.product_id = USB_PRODUCT_NVIDIA_SHIELD_CONTROLLER_V103;
    desc.name = "Virtual NVIDIA SHIELD Controller";
    device_id = SDL_AttachVirtualJoystick(&desc);
    SDLTest_AssertCheck(device_id > 0, "SDL_AttachVirtualJoystick() -> %" SDL_PRIs32 " (expected > 0)", device_id);
    SDLTest_AssertCheck(SDL_IsJoystickVirtual(device_id), "SDL_IsJoystickVirtual()");
    if (device_id > 0) {
        joystick = SDL_OpenJoystick(device_id);
        SDLTest_AssertCheck(joystick != NULL, "SDL_OpenJoystick()");
        if (joystick) {
            {
                const char *dname = SDL_GetJoystickName(joystick);
                SDLTest_AssertCheck(SDL_strcmp(dname, desc.name) == 0, "SDL_GetJoystickName() -> \"%s\" (expected \"%s\")", dname, desc.name);
            }
            {
                Uint16 vendor_id = SDL_GetJoystickVendor(joystick);
                SDLTest_AssertCheck(vendor_id == desc.vendor_id, "SDL_GetJoystickVendor() -> 0x%04x (expected 0x%04x)", vendor_id, desc.vendor_id);
            }
            {
                Uint16 product_id = SDL_GetJoystickProduct(joystick);
                SDLTest_AssertCheck(product_id == desc.product_id, "SDL_GetJoystickProduct() -> 0x%04x (expected 0x%04x)", product_id, desc.product_id);
            }
            {
                Uint16 product_version = SDL_GetJoystickProductVersion(joystick);
                SDLTest_AssertCheck(product_version == 0, "SDL_GetJoystickProductVersion() -> 0x%04x (expected 0x%04x)", product_version, 0);
            }
            {
                Uint16 firmware_Version = SDL_GetJoystickFirmwareVersion(joystick);
                SDLTest_AssertCheck(firmware_Version == 0, "SDL_GetJoystickFirmwareVersion() -> 0x%04x (expected 0x%04x)", firmware_Version, 0);
            }
            {
                const char *serial = SDL_GetJoystickSerial(joystick);
                SDLTest_AssertCheck(serial == NULL, "SDL_GetJoystickSerial() -> %s (expected %s)", serial, "(null)");
            }
            {
                SDL_JoystickType type = SDL_GetJoystickType(joystick);
                SDLTest_AssertCheck(type == desc.type, "SDL_GetJoystickType() -> %d (expected %d)", type, desc.type);
            }
            {
                Uint16 naxes = SDL_GetNumJoystickAxes(joystick);
                SDLTest_AssertCheck(naxes == desc.naxes, "SDL_GetNumJoystickAxes() -> 0x%04x (expected 0x%04x)", naxes, desc.naxes);
            }
            {
                int nballs = SDL_GetNumJoystickBalls(joystick);
                SDLTest_AssertCheck(nballs == 0, "SDL_GetNumJoystickBalls() -> %d (expected %d)", nballs, 0);
            }
            {
                int nhats = SDL_GetNumJoystickHats(joystick);
                SDLTest_AssertCheck(nhats == desc.nhats, "SDL_GetNumJoystickHats() -> %d (expected %d)", nhats, desc.nhats);
            }
            {
                int nbuttons = SDL_GetNumJoystickButtons(joystick);
                SDLTest_AssertCheck(nbuttons == desc.nbuttons, "SDL_GetNumJoystickButtons() -> %d (expected %d)", nbuttons, desc.nbuttons);
            }

            SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true), "SDL_SetJoystickVirtualButton(SDL_GAMEPAD_BUTTON_SOUTH, true)");
            SDL_UpdateJoysticks();
            SDLTest_AssertCheck(SDL_GetJoystickButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH) == true, "SDL_GetJoystickButton(SDL_GAMEPAD_BUTTON_SOUTH) == true");

            SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, false), "SDL_SetJoystickVirtualButton(SDL_GAMEPAD_BUTTON_SOUTH, false)");
            SDL_UpdateJoysticks();
            SDLTest_AssertCheck(SDL_GetJoystickButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH) == false, "SDL_GetJoystickButton(SDL_GAMEPAD_BUTTON_SOUTH) == false");

            gamepad = SDL_OpenGamepad(SDL_GetJoystickID(joystick));
            SDLTest_AssertCheck(gamepad != NULL, "SDL_OpenGamepad() succeeded");
            if (gamepad) {
                {
                    const char *name = SDL_GetGamepadName(gamepad);
                    SDLTest_AssertCheck(name && SDL_strcmp(name, desc.name) == 0, "SDL_GetGamepadName() -> \"%s\" (expected \"%s\")", name, desc.name);
                }
                {
                    Uint16 vendor_id = SDL_GetGamepadVendor(gamepad);
                    SDLTest_AssertCheck(vendor_id == desc.vendor_id, "SDL_GetGamepadVendor() -> 0x%04x (expected 0x%04x)", vendor_id, desc.vendor_id);
                }
                {
                    Uint16 product_id = SDL_GetGamepadProduct(gamepad);
                    SDLTest_AssertCheck(product_id == desc.product_id, "SDL_GetGamepadProduct() -> 0x%04x (expected 0x%04x)", product_id, desc.product_id);
                }

                /* Set an explicit mapping with a different name */
                SDL_SetGamepadMapping(SDL_GetJoystickID(joystick), "ff008316550900001072000000007601,Virtual Gamepad,a:b0,b:b1,x:b2,y:b3,back:b4,guide:b5,start:b6,leftstick:b7,rightstick:b8,leftshoulder:b9,rightshoulder:b10,dpup:b11,dpdown:b12,dpleft:b13,dpright:b14,misc1:b15,paddle1:b16,paddle2:b17,paddle3:b18,paddle4:b19,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,");
                {
                    const char *name = SDL_GetGamepadName(gamepad);
                    SDLTest_AssertCheck(name && SDL_strcmp(name, "Virtual Gamepad") == 0, "SDL_GetGamepadName() ->\"%s\" (expected \"%s\")", name, "Virtual Gamepad");
                }
                {
                    SDL_GamepadButtonLabel label = SDL_GetGamepadButtonLabel(gamepad, SDL_GAMEPAD_BUTTON_SOUTH);
                    SDLTest_AssertCheck(label == SDL_GAMEPAD_BUTTON_LABEL_A, "SDL_GetGamepadButtonLabel(SDL_GAMEPAD_BUTTON_SOUTH) -> %d (expected %d [%s])",
                                        label, SDL_GAMEPAD_BUTTON_LABEL_A, "SDL_GAMEPAD_BUTTON_LABEL_A");
                }
                SDLTest_AssertCheck(SDL_GetGamepadButtonLabel(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) == SDL_GAMEPAD_BUTTON_LABEL_A, "SDL_GetGamepadButtonLabel(SDL_GAMEPAD_BUTTON_SOUTH) == SDL_GAMEPAD_BUTTON_LABEL_A");

                /* Set the south button and verify that the gamepad responds */
                SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true), "SDL_SetJoystickVirtualButton(SDL_GAMEPAD_BUTTON_SOUTH, true)");
                SDL_UpdateJoysticks();
                SDLTest_AssertCheck(SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) == true, "SDL_GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == true");

                SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, false), "SDL_SetJoystickVirtualButton(SDL_GAMEPAD_BUTTON_SOUTH, false)");
                SDL_UpdateJoysticks();
                SDLTest_AssertCheck(SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) == false, "SDL_GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == false");

                /* Set an explicit mapping with legacy GameCube style buttons */
                SDL_SetGamepadMapping(SDL_GetJoystickID(joystick), "ff008316550900001072000000007601,Virtual Nintendo GameCube,a:b0,b:b1,x:b2,y:b3,back:b4,guide:b5,start:b6,leftstick:b7,rightstick:b8,leftshoulder:b9,rightshoulder:b10,dpup:b11,dpdown:b12,dpleft:b13,dpright:b14,misc1:b15,paddle1:b16,paddle2:b17,paddle3:b18,paddle4:b19,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,hint:SDL_GAMECONTROLLER_USE_GAMECUBE_LABELS:=1,");
                {
                    const char *name = SDL_GetGamepadName(gamepad);
                    SDLTest_AssertCheck(name && SDL_strcmp(name, "Virtual Nintendo GameCube") == 0, "SDL_GetGamepadName() -> \"%s\" (expected \"%s\")", name, "Virtual Nintendo GameCube");
                }
                SDLTest_AssertCheck(SDL_GetGamepadButtonLabel(gamepad, SDL_GAMEPAD_BUTTON_EAST) == SDL_GAMEPAD_BUTTON_LABEL_X, "SDL_GetGamepadButtonLabel(SDL_GAMEPAD_BUTTON_EAST) == SDL_GAMEPAD_BUTTON_LABEL_X");

                /* Set the east button and verify that the gamepad responds, mapping "B" to the west button */
                SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_EAST, true), "SDL_SetJoystickVirtualButton(SDL_GAMEPAD_BUTTON_EAST, true)");
                SDL_UpdateJoysticks();
                SDLTest_AssertCheck(SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST) == true, "SDL_GetGamepadButton(SDL_GAMEPAD_BUTTON_WEST) == true");

                SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_EAST, false), "SDL_SetJoystickVirtualButton(SDL_GAMEPAD_BUTTON_EAST, false)");
                SDL_UpdateJoysticks();
                SDLTest_AssertCheck(SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST) == false, "SDL_GetGamepadButton(SDL_GAMEPAD_BUTTON_WEST) == false");

                /* Set an explicit mapping with legacy Nintendo style buttons */
                SDL_SetGamepadMapping(SDL_GetJoystickID(joystick), "ff008316550900001072000000007601,Virtual Nintendo Gamepad,a:b1,b:b0,x:b3,y:b2,back:b4,guide:b5,start:b6,leftstick:b7,rightstick:b8,leftshoulder:b9,rightshoulder:b10,dpup:b11,dpdown:b12,dpleft:b13,dpright:b14,misc1:b15,paddle1:b16,paddle2:b17,paddle3:b18,paddle4:b19,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,hint:SDL_GAMECONTROLLER_USE_BUTTON_LABELS:=1,");
                {
                    const char *name = SDL_GetGamepadName(gamepad);
                    SDLTest_AssertCheck(name && SDL_strcmp(name, "Virtual Nintendo Gamepad") == 0, "SDL_GetGamepadName() -> \"%s\" (expected \"%s\")", name, "Virtual Nintendo Gamepad");
                }
                SDLTest_AssertCheck(SDL_GetGamepadButtonLabel(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) == SDL_GAMEPAD_BUTTON_LABEL_B, "SDL_GetGamepadButtonLabel(SDL_GAMEPAD_BUTTON_SOUTH) == SDL_GAMEPAD_BUTTON_LABEL_B");

                /* Set the south button and verify that the gamepad responds */
                SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true), "SDL_SetJoystickVirtualButton(SDL_GAMEPAD_BUTTON_SOUTH, true)");
                SDL_UpdateJoysticks();
                SDLTest_AssertCheck(SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) == true, "SDL_GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == true");

                SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, false), "SDL_SetJoystickVirtualButton(SDL_GAMEPAD_BUTTON_SOUTH, false)");
                SDL_UpdateJoysticks();
                SDLTest_AssertCheck(SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) == false, "SDL_GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == false");

                /* Set an explicit mapping with PS4 style buttons */
                SDL_SetGamepadMapping(SDL_GetJoystickID(joystick), "ff008316550900001072000000007601,Virtual PS4 Gamepad,type:ps4,a:b0,b:b1,x:b2,y:b3,back:b4,guide:b5,start:b6,leftstick:b7,rightstick:b8,leftshoulder:b9,rightshoulder:b10,dpup:b11,dpdown:b12,dpleft:b13,dpright:b14,misc1:b15,paddle1:b16,paddle2:b17,paddle3:b18,paddle4:b19,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,");
                {
                    const char *name = SDL_GetGamepadName(gamepad);
                    SDLTest_AssertCheck(SDL_strcmp(name, "Virtual PS4 Gamepad") == 0, "SDL_GetGamepadName() -> \"%s\" (expected \"%s\")", name, "Virtual PS4 Gamepad");
                }
                SDLTest_AssertCheck(SDL_GetGamepadButtonLabel(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) == SDL_GAMEPAD_BUTTON_LABEL_CROSS, "SDL_GetGamepadButtonLabel(SDL_GAMEPAD_BUTTON_SOUTH) == SDL_GAMEPAD_BUTTON_LABEL_CROSS");

                /* Set the south button and verify that the gamepad responds */
                SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true), "SDL_SetJoystickVirtualButton(SDL_GAMEPAD_BUTTON_SOUTH, true)");
                SDL_UpdateJoysticks();
                SDLTest_AssertCheck(SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) == true, "SDL_GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == true");

                SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, false), "SDL_SetJoystickVirtualButton(SDL_GAMEPAD_BUTTON_SOUTH, false)");
                SDL_UpdateJoysticks();
                SDLTest_AssertCheck(SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) == false, "SDL_GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == false");

                SDL_CloseGamepad(gamepad);
            }

            SDL_CloseJoystick(joystick);
        }
        SDLTest_AssertCheck(SDL_DetachVirtualJoystick(device_id), "SDL_DetachVirtualJoystick()");
    }
    SDLTest_AssertCheck(!SDL_IsJoystickVirtual(device_id), "!SDL_IsJoystickVirtual()");

    return TEST_COMPLETED;
}

/**
 * Check gamepad mappings
 */
static int SDLCALL joystick_testMappings(void *arg)
{
    SDL_VirtualJoystickDesc desc;
    SDL_Gamepad *gamepad = NULL;
    SDL_JoystickID device_id;

    /* Add a mapping for the virtual controller in advance */
    SDL_AddGamepadMapping("ff000000550900001472000000007601,Virtual Gamepad,a:b0,b:b1,x:b2,y:b3,back:b4,guide:b5,start:b6,leftstick:b7,rightstick:b8,leftshoulder:b9,rightshoulder:b10,dpup:b11,dpdown:b12,dpleft:b13,dpright:b14,misc1:b15,paddle1:b16,paddle2:b17,paddle3:b18,paddle4:b19,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,");

    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.vendor_id = USB_VENDOR_NVIDIA;
    desc.product_id = USB_PRODUCT_NVIDIA_SHIELD_CONTROLLER_V104;
    desc.name = "Virtual NVIDIA SHIELD Controller";
    device_id = SDL_AttachVirtualJoystick(&desc);
    SDLTest_AssertCheck(device_id > 0, "SDL_AttachVirtualJoystick() -> %" SDL_PRIs32 " (expected > 0)", device_id);
    SDLTest_AssertCheck(SDL_IsJoystickVirtual(device_id), "SDL_IsJoystickVirtual()");

    gamepad = SDL_OpenGamepad(device_id);
    SDLTest_AssertCheck(gamepad != NULL, "SDL_OpenGamepad() succeeded");
    if (gamepad) {
        /* Verify that the gamepad picked up the predefined mapping */
        {
            const char *name = SDL_GetGamepadName(gamepad);
            SDLTest_AssertCheck(name && SDL_strcmp(name, "Virtual Gamepad") == 0, "SDL_GetGamepadName() ->\"%s\" (expected \"%s\")", name, "Virtual Gamepad");
        }

        /* Verify that the gamepad picks up a new mapping with no CRC */
        SDL_AddGamepadMapping("ff000000550900001472000000007601,Virtual Gamepad V2,a:b0,b:b1,x:b2,y:b3,back:b4,guide:b5,start:b6,leftstick:b7,rightstick:b8,leftshoulder:b9,rightshoulder:b10,dpup:b11,dpdown:b12,dpleft:b13,dpright:b14,misc1:b15,paddle1:b16,paddle2:b17,paddle3:b18,paddle4:b19,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,");
        {
            const char *name = SDL_GetGamepadName(gamepad);
            SDLTest_AssertCheck(name && SDL_strcmp(name, "Virtual Gamepad V2") == 0, "SDL_GetGamepadName() ->\"%s\" (expected \"%s\")", name, "Virtual Gamepad V2");
        }

        /* Verify that the gamepad picks up a new mapping with valid CRC */
        SDL_AddGamepadMapping("ff008316550900001472000000007601,Virtual Gamepad V3,a:b0,b:b1,x:b2,y:b3,back:b4,guide:b5,start:b6,leftstick:b7,rightstick:b8,leftshoulder:b9,rightshoulder:b10,dpup:b11,dpdown:b12,dpleft:b13,dpright:b14,misc1:b15,paddle1:b16,paddle2:b17,paddle3:b18,paddle4:b19,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,");
        {
            const char *name = SDL_GetGamepadName(gamepad);
            SDLTest_AssertCheck(name && SDL_strcmp(name, "Virtual Gamepad V3") == 0, "SDL_GetGamepadName() ->\"%s\" (expected \"%s\")", name, "Virtual Gamepad V3");
        }

        SDL_CloseGamepad(gamepad);
    }
    SDLTest_AssertCheck(SDL_DetachVirtualJoystick(device_id), "SDL_DetachVirtualJoystick()");

    /* Try matching mappings with a new CRC */
    desc.name = "Virtual NVIDIA SHIELD Controller V2";
    device_id = SDL_AttachVirtualJoystick(&desc);
    SDLTest_AssertCheck(device_id > 0, "SDL_AttachVirtualJoystick() -> %" SDL_PRIs32 " (expected > 0)", device_id);
    SDLTest_AssertCheck(SDL_IsJoystickVirtual(device_id), "SDL_IsJoystickVirtual()");

    gamepad = SDL_OpenGamepad(device_id);
    SDLTest_AssertCheck(gamepad != NULL, "SDL_OpenGamepad() succeeded");
    if (gamepad) {
        {
            const char *name = SDL_GetGamepadName(gamepad);
            SDLTest_AssertCheck(name && SDL_strcmp(name, "Virtual Gamepad V2") == 0, "SDL_GetGamepadName() ->\"%s\" (expected \"%s\")", name, "Virtual Gamepad V2");
        }
        SDL_CloseGamepad(gamepad);
    }
    SDLTest_AssertCheck(SDL_DetachVirtualJoystick(device_id), "SDL_DetachVirtualJoystick()");

    return TEST_COMPLETED;
}

/**
 * Check that device and input event lifecycles stay consistent when
 * gamepads are unplugged and replugged while input is still pending.
 */
static int SDLCALL joystick_testHotplugEvents(void *arg)
{
    SDL_VirtualJoystickDesc desc;
    SDL_JoystickID device_a = 0, device_b = 0, device_c = 0;
    SDL_Gamepad *gamepad_a = NULL, *gamepad_b = NULL, *gamepad_c = NULL;
    int player_a, player_b;
    SDL_Event event;
    bool joystick_removed = false;
    bool gamepad_removed = false;
    bool events_after_removed = false;

    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.vendor_id = USB_VENDOR_NVIDIA;
    desc.product_id = USB_PRODUCT_NVIDIA_SHIELD_CONTROLLER_V103;
    desc.name = "Virtual NVIDIA SHIELD Controller";

    /* Make sure no stale events (or a leftover poll sentinel) are pending */
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

    device_a = SDL_AttachVirtualJoystick(&desc);
    SDLTest_AssertCheck(device_a > 0, "SDL_AttachVirtualJoystick() -> %" SDL_PRIs32 " (expected > 0)", device_a);
    device_b = SDL_AttachVirtualJoystick(&desc);
    SDLTest_AssertCheck(device_b > 0, "SDL_AttachVirtualJoystick() -> %" SDL_PRIs32 " (expected > 0)", device_b);
    if (!device_a || !device_b) {
        return TEST_ABORTED;
    }
    SDLTest_AssertCheck(device_a != device_b, "attached gamepads have distinct instance IDs");

    gamepad_a = SDL_OpenGamepad(device_a);
    SDLTest_AssertCheck(gamepad_a != NULL, "SDL_OpenGamepad() for the first gamepad");
    gamepad_b = SDL_OpenGamepad(device_b);
    SDLTest_AssertCheck(gamepad_b != NULL, "SDL_OpenGamepad() for the second gamepad");
    if (!gamepad_a || !gamepad_b) {
        return TEST_ABORTED;
    }

    player_a = SDL_GetGamepadPlayerIndex(gamepad_a);
    player_b = SDL_GetGamepadPlayerIndex(gamepad_b);
    SDLTest_AssertCheck(player_a >= 0 && player_b >= 0 && player_a != player_b,
                        "gamepads have distinct player indices (%d and %d)", player_a, player_b);

    /* Queue input on both gamepads */
    SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(SDL_GetGamepadJoystick(gamepad_a), SDL_GAMEPAD_BUTTON_SOUTH, true),
                        "SDL_SetJoystickVirtualButton() on the first gamepad");
    SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(SDL_GetGamepadJoystick(gamepad_b), SDL_GAMEPAD_BUTTON_SOUTH, true),
                        "SDL_SetJoystickVirtualButton() on the second gamepad");
    SDL_UpdateJoysticks();

    /* Unplug the first gamepad and plug in a new one while input is pending */
    SDLTest_AssertCheck(SDL_DetachVirtualJoystick(device_a), "SDL_DetachVirtualJoystick()");
    device_c = SDL_AttachVirtualJoystick(&desc);
    SDLTest_AssertCheck(device_c > 0, "SDL_AttachVirtualJoystick() -> %" SDL_PRIs32 " (expected > 0)", device_c);
    SDLTest_AssertCheck(device_c != device_a && device_c != device_b,
                        "reconnected gamepad gets a new instance ID (%" SDL_PRIs32 ")", device_c);

    /* Drain the event queue: nothing may arrive for the removed gamepad
     * after its removal events have been delivered. */
    SDL_UpdateJoysticks();
    SDL_PumpEvents();
    while (SDL_PollEvent(&event)) {
        Uint32 which = 0;
        bool joystick_event = false;
        bool gamepad_event = false;

        switch (event.type) {
        case SDL_EVENT_JOYSTICK_ADDED:
        case SDL_EVENT_JOYSTICK_REMOVED:
        case SDL_EVENT_JOYSTICK_UPDATE_COMPLETE:
            which = event.jdevice.which;
            joystick_event = true;
            break;
        case SDL_EVENT_JOYSTICK_AXIS_MOTION:
            which = event.jaxis.which;
            joystick_event = true;
            break;
        case SDL_EVENT_JOYSTICK_BALL_MOTION:
            which = event.jball.which;
            joystick_event = true;
            break;
        case SDL_EVENT_JOYSTICK_HAT_MOTION:
            which = event.jhat.which;
            joystick_event = true;
            break;
        case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
        case SDL_EVENT_JOYSTICK_BUTTON_UP:
            which = event.jbutton.which;
            joystick_event = true;
            break;
        case SDL_EVENT_JOYSTICK_BATTERY_UPDATED:
            which = event.jbattery.which;
            joystick_event = true;
            break;
        case SDL_EVENT_GAMEPAD_ADDED:
        case SDL_EVENT_GAMEPAD_REMOVED:
        case SDL_EVENT_GAMEPAD_REMAPPED:
        case SDL_EVENT_GAMEPAD_UPDATE_COMPLETE:
        case SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED:
            which = event.gdevice.which;
            gamepad_event = true;
            break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            which = event.gaxis.which;
            gamepad_event = true;
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            which = event.gbutton.which;
            gamepad_event = true;
            break;
        default:
            break;
        }

        if (event.type == SDL_EVENT_JOYSTICK_REMOVED && which == device_a) {
            joystick_removed = true;
        } else if (event.type == SDL_EVENT_GAMEPAD_REMOVED && which == device_a) {
            gamepad_removed = true;
        } else if (which == device_a) {
            if ((joystick_event && joystick_removed) || (gamepad_event && gamepad_removed)) {
                events_after_removed = true;
            }
        }
    }
    SDLTest_AssertCheck(joystick_removed, "SDL_EVENT_JOYSTICK_REMOVED received for the unplugged gamepad");
    SDLTest_AssertCheck(gamepad_removed, "SDL_EVENT_GAMEPAD_REMOVED received for the unplugged gamepad");
    SDLTest_AssertCheck(!events_after_removed, "no events for the unplugged gamepad after its removal events");

    /* The gamepad that stayed connected is unaffected */
    SDLTest_AssertCheck(SDL_GamepadConnected(gamepad_b), "second gamepad is still connected");
    SDLTest_AssertCheck(SDL_GetGamepadPlayerIndex(gamepad_b) == player_b,
                        "second gamepad kept its player index (%d)", player_b);
    SDLTest_AssertCheck(SDL_GetGamepadButton(gamepad_b, SDL_GAMEPAD_BUTTON_SOUTH) == true,
                        "second gamepad kept its input state");

    /* The unplugged gamepad is offline and recentered */
    SDLTest_AssertCheck(!SDL_GamepadConnected(gamepad_a), "first gamepad is no longer connected");
    SDLTest_AssertCheck(SDL_GetGamepadPlayerIndex(gamepad_a) == -1,
                        "first gamepad no longer has a player index");
    SDLTest_AssertCheck(SDL_GetGamepadButton(gamepad_a, SDL_GAMEPAD_BUTTON_SOUTH) == false,
                        "first gamepad was recentered");

    /* The new gamepad only presents its own fresh state */
    SDLTest_AssertCheck(SDL_IsGamepad(device_c), "new gamepad is recognized as a gamepad");
    gamepad_c = SDL_OpenGamepad(device_c);
    SDLTest_AssertCheck(gamepad_c != NULL, "SDL_OpenGamepad() for the new gamepad");
    if (gamepad_c) {
        SDLTest_AssertCheck(SDL_GetGamepadPlayerIndex(gamepad_c) == player_a,
                            "new gamepad got the freed player index (%d)", player_a);
        SDLTest_AssertCheck(SDL_GetGamepadButton(gamepad_c, SDL_GAMEPAD_BUTTON_SOUTH) == false,
                            "new gamepad starts with fresh input state");
        SDL_CloseGamepad(gamepad_c);
    }

    SDL_CloseGamepad(gamepad_a);
    SDL_CloseGamepad(gamepad_b);
    SDLTest_AssertCheck(SDL_DetachVirtualJoystick(device_b), "SDL_DetachVirtualJoystick()");
    SDLTest_AssertCheck(SDL_DetachVirtualJoystick(device_c), "SDL_DetachVirtualJoystick()");

    return TEST_COMPLETED;
}

/**
 * Missing joystick axes must not become half-pressed gamepad triggers.
 */
static int SDLCALL joystick_testMissingAxes(void *arg)
{
    SDL_VirtualJoystickDesc joystick_desc;
    SDL_JoystickID joystick_id = 0;
    SDL_Gamepad *gamepad = NULL;
    SDL_Joystick *joystick = NULL;
    char guid_string[33] = {0};
    char mapping_string[256] = {0};

    SDL_INIT_INTERFACE(&joystick_desc);
    joystick_desc.type = SDL_JOYSTICK_TYPE_FLIGHT_STICK;
    joystick_desc.naxes = 2;
    joystick_desc.nbuttons = 2;
    joystick_desc.name = "Virtual two-axis joystick";
    joystick_id = SDL_AttachVirtualJoystick(&joystick_desc);
    SDLTest_AssertCheck(joystick_id != 0, "SDL_AttachVirtualJoystick()");
    if (!joystick_id) {
        return TEST_ABORTED;
    }

    SDL_GUIDToString(SDL_GetJoystickGUIDForID(joystick_id), guid_string, sizeof(guid_string));
    /* Axis 2 is the first invalid index; axis 5 matches the DOS fallback mapping.
     * A second right-trigger binding checks that valid bindings are still read. */
    SDL_snprintf(mapping_string, sizeof(mapping_string),
                 "%s,Virtual two-axis joystick,leftx:a0,lefty:a1,a:b0,"
                 "lefttrigger:a2,righttrigger:a5,righttrigger:b1,", guid_string);
    SDLTest_AssertCheck(SDL_SetGamepadMapping(joystick_id, mapping_string), "SDL_SetGamepadMapping()");
    gamepad = SDL_OpenGamepad(joystick_id);
    SDLTest_AssertCheck(gamepad != NULL, "SDL_OpenGamepad()");
    if (gamepad) {
        joystick = SDL_GetGamepadJoystick(gamepad);
        SDLTest_AssertCheck(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) == 0,
                            "Missing left trigger is released");
        SDLTest_AssertCheck(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) == 0,
                            "Missing right trigger is released");

        SDLTest_AssertCheck(SDL_SetJoystickVirtualAxis(joystick, 0, -16384), "Set X axis");
        SDLTest_AssertCheck(SDL_SetJoystickVirtualAxis(joystick, 1, 16384), "Set Y axis");
        SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(joystick, 0, true), "Press button 0");
        SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(joystick, 1, true), "Press button 1");
        SDL_UpdateJoysticks();
        SDLTest_AssertCheck(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX) == -16384, "X axis is unchanged");
        SDLTest_AssertCheck(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY) == 16384, "Y axis is unchanged");
        SDLTest_AssertCheck(SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH), "Button 0 is pressed");
        SDLTest_AssertCheck(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) == SDL_JOYSTICK_AXIS_MAX,
                            "Valid button binding works after a missing axis");
        SDLTest_AssertCheck(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) == 0,
                            "Missing left trigger stays released while other controls move");

        SDLTest_AssertCheck(SDL_SetJoystickVirtualButton(joystick, 1, false), "Release button 1");
        SDL_UpdateJoysticks();
        SDLTest_AssertCheck(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) == 0,
                            "Right trigger returns to released");
        SDL_CloseGamepad(gamepad);
    }
    SDLTest_AssertCheck(SDL_DetachVirtualJoystick(joystick_id), "SDL_DetachVirtualJoystick()");
    return TEST_COMPLETED;
}

/* ================= Test References ================== */

/* Joystick routine test cases */
static const SDLTest_TestCaseReference joystickTest1 = {
    joystick_testVirtual, "joystick_testVirtual", "Test virtual joystick functionality", TEST_ENABLED
};
static const SDLTest_TestCaseReference joystickTest2 = {
    joystick_testMappings, "joystick_testMappings", "Test gamepad mapping functionality", TEST_ENABLED
};
static const SDLTest_TestCaseReference joystickTest3 = {
    joystick_testMissingAxes, "joystick_testMissingAxes", "Test mappings with missing joystick axes", TEST_ENABLED
};
static const SDLTest_TestCaseReference joystickTest4 = {
    joystick_testHotplugEvents, "joystick_testHotplugEvents", "Test device and event lifecycle across hotplug", TEST_ENABLED
};

/* Sequence of Joystick routine test cases */
static const SDLTest_TestCaseReference *joystickTests[] = {
    &joystickTest1,
    &joystickTest2,
    &joystickTest3,
    &joystickTest4,
    NULL
};

/* Joystick routine test suite (global) */
SDLTest_TestSuiteReference joystickTestSuite = {
    "Joystick",
    joystickSetUp,
    joystickTests,
    joystickTearDown
};
