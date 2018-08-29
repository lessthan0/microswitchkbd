/* 
  Copyright 2014 Mike McMahon

  LUFA Library
  Copyright 2014  Dean Camera (dean [at] fourwalledcubicle [dot] com)
  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/* ========================================================================
   $File: MicroSwitch Keyboard
   $Date: 2018-08-27
   $Revision: .5
   $Creator: Chase Curtis $
   $Notice: (C) Copyright 2014 by Dark Arts, Inc. All Rights Reserved. $
   ======================================================================== */


#include "keydriver.h"

static USB_KeyboardReport_Data_t PrevKeyboardReport;


USB_ClassInfo_HID_Device_t Keyboard_HID_Interface =
{ 
  .Config =
  {
    .InterfaceNumber        = INTERFACE_ID_Keyboard,
    .ReportINEndpoint       =
    {
      .Address              = KEYBOARD_EPADDR,
      .Size                 = KEYBOARD_EPSIZE,
      .Banks                = 1,
    },
    .PrevReportINBuffer     = &PrevKeyboardReport,
    .PrevReportINBufferSize = sizeof(PrevKeyboardReport),
  },
};

typedef enum {
  NONE = 0,
  L_SHIFT = 1, R_SHIFT, L_CONTROL, R_CONTROL, L_META, R_META, L_SUPER, R_SUPER,
  CAPS_LOCK,
} KeyShift;



#define SHIFT(s) (1L << s)

#define L_ALT L_META
#define R_ALT R_META
#define L_GUI L_SUPER
#define R_GUI R_SUPER

#define MAX_USB_SHIFT R_GUI

typedef uint8_t HidUsageID;

// Information about each key.
typedef struct
{
  HidUsageID hidUsageID;        // Currently always from the Keyboard / Keypad page.
  KeyShift shift;
  PGM_P keysym;
// Or NULL if an ordinary PC/AT-101 key with no symbol.
} KeyInfo;

// As much as possible, keysyms are taken from <gdk/gdkkeysyms.h>,
// which seems to be the most comprehensive list of X keysyms.
#define KEYSYM(name,keysym) static const char name[] PROGMEM = keysym
#define NO_KEY(idx) {0,NONE,NULL}
#define SHIFT_KEY(idx,hid,shift) { hid, shift, NULL}

#define PC_KEY(idx,hid,keysym) {hid,NONE,keysym}

typedef enum {
  HUT1 = 1
} TranslationMode;


static TranslationMode CurrentModes[1];


static uint32_t CurrentShifts;
static HidUsageID KeysDown[16];
static uint8_t NKeysDown;
static bool NeedEmptyReport;


#define SC_ADDR_DDR DDRB
#define SC_ADDR_PORT PORTB
#define SC_ADDR_SHIFT 4
#define SC_STROBE_DDR DDRB
#define SC_STROBE_PORT PORTB
#define SC_STROBE (1 << 0)
#define SC_KEYS_PIN PIND


static uint8_t DirectKeyStates[16], DirectNKeyStates[16];

static void KeyDown(const KeyInfo *key, bool noKeyUps);
static void KeyUp(const KeyInfo *key);
static uint8_t Direct_Read(uint8_t column);
static void Direct_Init(void);
static void Direct_Scan(void);
//static bool IsKeyDown(HidUsageID key);
static void AddKeyReport(USB_KeyboardReport_Data_t* KeyboardReport);
void SetupHardware(void);
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const,
                                         uint8_t* const,
                                         const uint8_t ,
                                         void* , uint16_t* const);
#define LOW 0
#define HIGH 1

static const KeyInfo Keys[128] PROGMEM = {
    

    PC_KEY(000, HID_KEYBOARD_SC_DOT_AND_GREATER_THAN_SIGN,NULL), //>
    PC_KEY(001, HID_KEYBOARD_SC_SEMICOLON_AND_COLON, NULL), // HELP
    PC_KEY(002, HID_KEYBOARD_SC_P, NULL),
    PC_KEY(003, HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS, NULL), // CAPS-LOCK
    PC_KEY(004, HID_KEYBOARD_SC_COMMA_AND_LESS_THAN_SIGN, NULL), // BOLD-LOCK (shift key? LED?)
    PC_KEY(005, HID_KEYBOARD_SC_L, NULL), //l
    PC_KEY(006, HID_KEYBOARD_SC_O, NULL), // O
    PC_KEY(007, HID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS, NULL), // 9
    PC_KEY(010, HID_KEYBOARD_SC_SPACE, NULL),
    PC_KEY(011, HID_KEYBOARD_SC_Z, NULL), // 
    PC_KEY(012, HID_KEYBOARD_SC_BACKSLASH_AND_PIPE, NULL), // Backslash
    PC_KEY(013, HID_KEYBOARD_SC_KEYPAD_9_AND_PAGE_UP, NULL), // 9 and )
    PC_KEY(014,HID_KEYBOARD_SC_Z,NULL),
    PC_KEY(015, HID_KEYBOARD_SC_A, NULL), // A
    PC_KEY(016, HID_KEYBOARD_SC_Q, NULL), // Q
    PC_KEY(017, HID_KEYBOARD_SC_1_AND_EXCLAMATION, NULL), //1 and !
    PC_KEY(018, HID_KEYBOARD_SC_TAB, NULL), // TAB
    PC_KEY(019,HID_KEYBOARD_SC_X,NULL), 
    PC_KEY(022,HID_KEYBOARD_SC_Y,NULL), 
    PC_KEY(023, HID_KEYBOARD_SC_GRAVE_ACCENT_AND_TILDE, NULL), // `
    PC_KEY(024, HID_KEYBOARD_SC_V, NULL), // V
    PC_KEY(025, HID_KEYBOARD_SC_G, NULL), // G
    PC_KEY(026, HID_KEYBOARD_SC_T, NULL), // T
    PC_KEY(027, HID_KEYBOARD_SC_5_AND_PERCENTAGE, NULL), // F4
    PC_KEY(028,HID_KEYBOARD_SC_G, NULL), 
    PC_KEY(029,HID_KEYBOARD_SC_H, NULL),
    PC_KEY(030,HID_KEYBOARD_SC_I, NULL), 
    PC_KEY(033,HID_KEYBOARD_SC_3_AND_HASHMARK, NULL),//3 and pound 
    PC_KEY(034, HID_KEYBOARD_SC_6_AND_CARET , NULL), // LEFT-CONTROL
    PC_KEY(035, HID_KEYBOARD_SC_HOME, NULL ), // RIGHT-CONTROL
    PC_KEY(036,HID_KEYBOARD_SC_Y, NULL),//Y 
    PC_KEY(037,HID_KEYBOARD_SC_O, NULL),
    PC_KEY(038,HID_KEYBOARD_SC_O, NULL),
    PC_KEY(041, HID_KEYBOARD_SC_BACKSLASH_AND_PIPE, NULL), // RESUME
    PC_KEY(042, HID_KEYBOARD_SC_CLOSING_BRACKET_AND_CLOSING_BRACE, NULL),
    PC_KEY(043, HID_KEYBOARD_SC_KEYPAD_ASTERISK, NULL), // ALT (ESCAPE actually?)
    PC_KEY(044, HID_KEYBOARD_SC_N, NULL), // N
    PC_KEY(045, HID_KEYBOARD_SC_J, NULL), // J
    PC_KEY(046, HID_KEYBOARD_SC_U, NULL), // U
    PC_KEY(047, HID_KEYBOARD_SC_7_AND_AMPERSAND,NULL), //7 and backtick
    PC_KEY(048, HID_KEYBOARD_SC_B,NULL), 
    PC_KEY(049, HID_KEYBOARD_SC_C,NULL), 
    PC_KEY(050, HID_KEYBOARD_SC_D,NULL), 
    PC_KEY(053, HID_KEYBOARD_SC_O,NULL), // broken Key
    PC_KEY(054, HID_KEYBOARD_SC_E,NULL), 
    PC_KEY(055, HID_KEYBOARD_SC_F,NULL), 
    PC_KEY(056, HID_KEYBOARD_SC_G,NULL), 
    PC_KEY(057,HID_KEYBOARD_SC_H,NULL), 
    PC_KEY(058,HID_KEYBOARD_SC_I,NULL), 
    PC_KEY(061, HID_KEYBOARD_SC_KEYPAD_2_AND_DOWN_ARROW, NULL), // 2
    PC_KEY(062, HID_KEYBOARD_SC_KEYPAD_5, NULL), // 5
    PC_KEY(063, HID_KEYBOARD_SC_KEYPAD_7_AND_HOME, NULL), // 7
    PC_KEY(064, HID_KEYBOARD_SC_X, NULL),//D 
    PC_KEY(064, HID_KEYBOARD_SC_D, NULL),//X 
    PC_KEY(066, HID_KEYBOARD_SC_E, NULL), // E
    PC_KEY(067, HID_KEYBOARD_SC_3_AND_HASHMARK,NULL), //num 3
    PC_KEY(068, HID_KEYBOARD_SC_K,NULL),  
    PC_KEY(071, HID_KEYBOARD_SC_O, NULL), 
    PC_KEY(072, HID_KEYBOARD_SC_F14, NULL ),//f14 
    PC_KEY(073, HID_KEYBOARD_SC_F13, NULL), //f13
    PC_KEY(074, HID_KEYBOARD_SC_F12, NULL), //f12
    PC_KEY(075, HID_KEYBOARD_SC_F11, NULL), //f11
    PC_KEY(076, HID_KEYBOARD_SC_F10, NULL), //F10
    PC_KEY(077, HID_KEYBOARD_SC_F9, NULL), //F9
    PC_KEY(100, HID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK, NULL),//forward slash
    PC_KEY(101, HID_KEYBOARD_SC_H, NULL), //check is this real b??
    PC_KEY(102, HID_KEYBOARD_SC_OPENING_BRACKET_AND_OPENING_BRACE, NULL), //Bracket
    PC_KEY(103, HID_KEYBOARD_SC_B, NULL), //B
    PC_KEY(104, HID_KEYBOARD_SC_M, NULL), //M
    PC_KEY(105, HID_KEYBOARD_SC_K, NULL),//K
    PC_KEY(106, HID_KEYBOARD_SC_I, NULL), //I
    PC_KEY(107, HID_KEYBOARD_SC_8_AND_ASTERISK, NULL), //8
    SHIFT_KEY(145, HID_KEYBOARD_SC_RIGHT_GUI, R_SUPER), //right shift
    SHIFT_KEY(165, HID_KEYBOARD_SC_RIGHT_ALT, R_META), //rept
    SHIFT_KEY(020, HID_KEYBOARD_SC_LEFT_CONTROL, L_CONTROL), // left control 
    PC_KEY(113,HID_KEYBOARD_SC_K, NULL), //
    SHIFT_KEY(114, HID_KEYBOARD_SC_LEFT_SHIFT, L_SHIFT), //SHIFT
    SHIFT_KEY(125, HID_KEYBOARD_SC_RIGHT_SHIFT, R_SHIFT),//SHIFT LOCK
    PC_KEY(116, HID_KEYBOARD_SC_TAB, NULL), //TAB
    SHIFT_KEY(125, HID_KEYBOARD_SC_CAPS_LOCK, CAPS_LOCK), //caps lock
    PC_KEY(121, HID_KEYBOARD_SC_KEYPAD_0_AND_INSERT, NULL), //num 0
    PC_KEY(122,HID_KEYBOARD_SC_KEYPAD_1_AND_END, NULL), //num 1
    PC_KEY(123, HID_KEYBOARD_SC_KEYPAD_4_AND_LEFT_ARROW, NULL), //num 4 
    PC_KEY(124, HID_KEYBOARD_SC_M, NULL), // 
    PC_KEY(125, HID_KEYBOARD_SC_C, NULL), // C
    PC_KEY(126, HID_KEYBOARD_SC_F, NULL), // F
    PC_KEY(127, HID_KEYBOARD_SC_R, NULL), // R
    PC_KEY(130, HID_KEYBOARD_SC_4_AND_DOLLAR, NULL), //num 4 
    PC_KEY(131,HID_KEYBOARD_SC_X, NULL), //
    PC_KEY(132,HID_KEYBOARD_SC_Y, NULL), //
    PC_KEY(133,HID_KEYBOARD_SC_Z, NULL), //
    PC_KEY(134,HID_KEYBOARD_SC_O, NULL), //
    PC_KEY(135, HID_KEYBOARD_SC_KEYPAD_2_AND_DOWN_ARROW, NULL), // down 
    PC_KEY(136,HID_KEYBOARD_SC_N, NULL), // 
    PC_KEY(137, HID_KEYBOARD_SC_BACKSPACE, NULL), // 
    PC_KEY(140,HID_KEYBOARD_SC_P, NULL), // 
    PC_KEY(141, HID_KEYBOARD_SC_LEFT_ARROW, NULL), // 
    PC_KEY(142, HID_KEYBOARD_SC_ENTER, NULL), // enter
    PC_KEY(143, HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE, NULL), //minus 
    PC_KEY(144,HID_KEYBOARD_SC_DELETE, NULL),//bell off, *delete
    PC_KEY(145,HID_KEYBOARD_SC_EQUAL_AND_PLUS, NULL),//equal dash
    PC_KEY(146,HID_KEYBOARD_SC_KEYPAD_6_AND_RIGHT_ARROW, NULL), //right arrow

// SHIFT_KEY(147, HID_KEYBOARD_SC_LEFT_SHIFT, L_SHIFT), // LEFT-SHIFT
    PC_KEY(150, HID_KEYBOARD_SC_KEYPAD_8_AND_UP_ARROW, NULL), //up arrow
    PC_KEY(151, HID_KEYBOARD_SC_DOWN_ARROW, NULL), //down arrow
    PC_KEY(152,HID_KEYBOARD_SC_S, NULL), //
    PC_KEY(153,HID_KEYBOARD_SC_T, NULL), //
    PC_KEY(154,HID_KEYBOARD_SC_U, NULL), //
    PC_KEY(155,HID_KEYBOARD_SC_V, NULL), //
    PC_KEY(156, HID_KEYBOARD_SC_RIGHT_ARROW, NULL), //edit right
    PC_KEY(157, HID_KEYBOARD_SC_H, NULL), //colon
    PC_KEY(160, HID_KEYBOARD_SC_KEYPAD_8_AND_UP_ARROW, NULL), //up arrow
    PC_KEY(161, HID_KEYBOARD_SC_O, NULL), //
    PC_KEY(162, HID_KEYBOARD_SC_KEYPAD_DOT_AND_DELETE, NULL), // period
    PC_KEY(163,HID_KEYBOARD_SC_KEYPAD_3_AND_PAGE_DOWN, NULL),//num 3
    PC_KEY(164,HID_KEYBOARD_SC_Q, NULL), //  
    PC_KEY(165, HID_KEYBOARD_SC_KEYPAD_8_AND_UP_ARROW, NULL), //num 8 
    PC_KEY(166, HID_KEYBOARD_SC_Z, NULL), // Z
    PC_KEY(167, HID_KEYBOARD_SC_S, NULL), // S
    PC_KEY(170, HID_KEYBOARD_SC_W, NULL), // W
    PC_KEY(171,HID_KEYBOARD_SC_2_AND_AT, NULL),//2
    PC_KEY(172,HID_KEYBOARD_SC_F16, NULL),//F1
    PC_KEY(173, HID_KEYBOARD_SC_F20, NULL), //F2 
    PC_KEY(174,HID_KEYBOARD_SC_F18, NULL),//F3
    PC_KEY(175, HID_KEYBOARD_SC_STOP, NULL), //F4 
    PC_KEY(176, HID_KEYBOARD_SC_F5, NULL), // last page
    PC_KEY(177, HID_KEYBOARD_SC_ESCAPE, NULL) // end

    };


static bool NonLockingKeyDown(void)
{
  int i;
  for (i = 0; i < NKeysDown; i++)
  {
    switch (KeysDown[i])
    {
    case HID_KEYBOARD_SC_LOCKING_CAPS_LOCK:
    case HID_KEYBOARD_SC_LOCKING_NUM_LOCK:
    case HID_KEYBOARD_SC_LOCKING_SCROLL_LOCK:
      break;
    default:
      return true;
    }
  }
  return false;
}


static void KeyDown(/*PROGMEM*/ const KeyInfo *key, bool noKeyUps)
{
  HidUsageID usage = pgm_read_byte(&key->hidUsageID);
  KeyShift shift = pgm_read_byte(&key->shift);
  
  

  if (noKeyUps)
  {
    NKeysDown = 0;
  }
    if (shift != NONE)
    {
      CurrentShifts |= SHIFT(shift);
      if (shift <= MAX_USB_SHIFT)
        return;                  // No need for usage entry.
    }

    if (noKeyUps)
    {
#define MAP_SPECIAL_SHIFT(u,s)          \
        if (CurrentShifts & SHIFT(s))   \
        {                               \
            KeysDown[NKeysDown++] = u;  \
        }                               \

      MAP_SPECIAL_SHIFT(HID_KEYBOARD_SC_LOCKING_CAPS_LOCK,CAPS_LOCK);
    }
    if (NKeysDown < sizeof(KeysDown))
    {
      KeysDown[NKeysDown++] = usage;
    }
    

}

static void AddKeyReport(USB_KeyboardReport_Data_t* KeyboardReport)
{
  
  uint8_t shifts;
  int i;

  // Do not even send shifts; they could be out-of-date until the next key down.
 
#define ADD_SHIFT(m,s)          \
  if (CurrentShifts & SHIFT(s)) \
    shifts |= m;\

  shifts = 0;

  ADD_SHIFT(HID_KEYBOARD_MODIFIER_LEFTCTRL,L_CONTROL);
  ADD_SHIFT(HID_KEYBOARD_MODIFIER_LEFTSHIFT,L_SHIFT);
  ADD_SHIFT(HID_KEYBOARD_MODIFIER_LEFTALT,R_ALT);
  ADD_SHIFT(HID_KEYBOARD_MODIFIER_LEFTGUI,R_GUI);
 ADD_SHIFT(HID_KEYBOARD_MODIFIER_RIGHTSHIFT,R_SHIFT); 
  KeyboardReport->Modifier = shifts;


  if (NKeysDown > sizeof(KeyboardReport->KeyCode))
  {
    for (i = 0; i < sizeof(KeyboardReport->KeyCode); i++)
    {
      KeyboardReport->KeyCode[i] = HID_KEYBOARD_SC_ERROR_ROLLOVER;
    }
  }
  else
  {
    for (i = 0; i < NKeysDown; i++)
    {
      KeyboardReport->KeyCode[i] = KeysDown[i];
    }
  }
 
}



static uint8_t Direct_Read(uint8_t column)
{
  uint8_t p2;

  SC_ADDR_PORT = (SC_ADDR_PORT & ~(0x0F << SC_ADDR_SHIFT)) | (column << SC_ADDR_SHIFT);
  SC_STROBE_PORT &= ~SC_STROBE;


  _delay_us(5);


  p2 = SC_KEYS_PIN;

  SC_STROBE_PORT |= SC_STROBE;
  return p2;
}

static void Direct_Init(void)
{
  int i;

  SC_ADDR_DDR |= (0x0F << SC_ADDR_SHIFT);
  SC_STROBE_DDR |= SC_STROBE;
  SC_STROBE_PORT |= SC_STROBE;  // Idle high.

  for (i = 0; i < 16; i++)
    DirectKeyStates[i] = 1;
 
}

static void Direct_Scan(void)
{
  int i;
  int j;
  
  for (i = 0; i < 16; i++)
  {
    DirectNKeyStates[i] = Direct_Read(i);
  }

  for (i = 0; i < 16; i++)
  {
      uint8_t keys;
      uint8_t change;
      
    keys =   DirectNKeyStates[i];
    change = keys ^ DirectKeyStates[i];

    if (change == 0) continue;
      DirectKeyStates[i] = keys;

      for (j = 0; j < 8; j++)
      {
          if (change & (1 << j))
          {
              int code = (i * 8) + j;
              if (keys & (1 << j))
              {
                  KeyUp(&Keys[code]);
              }
              else
              {
                  KeyDown(&Keys[code],false);
             
              }
          }
      }
  }
}


 static void KeyUp(/*PROGMEM*/ const KeyInfo *key)
 {
  HidUsageID usage = pgm_read_byte(&key->hidUsageID);
  KeyShift shift = pgm_read_byte(&key->shift);
  int i;

  if (shift != NONE)
  {
    CurrentShifts &= ~SHIFT(shift);
  }

  for (i = 0; i < NKeysDown; i++)
  {

      if (KeysDown[i] == usage)
      {

          NKeysDown--;
          while (i < NKeysDown)
          {
              KeysDown[i] = KeysDown[i+1];
              i++;
          }
          break;
      }

  }
 }
 
int main(void)                     
{

      
     
      SetupHardware();   
      GlobalInterruptEnable();
    

  while(true)
  {
    Direct_Scan();
    HID_Device_USBTask(&Keyboard_HID_Interface);
    USB_USBTask();
  }



}


/* Configures the board hardware and keyboard pins. */
 void SetupHardware(void)
{
#if (ARCH == ARCH_AVR8)
  /* Disable watchdog if enabled by bootloader/fuses */
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

  /* Disable clock division */
  clock_prescale_set(clock_div_1);
#elif (ARCH == ARCH_XMEGA)
  /* Start the PLL to multiply the 2MHz RC oscillator to 32MHz and switch the CPU core to run from it */
   XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
  XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);

  /* Start the 32MHz internal RC oscillator and start the DFLL to increase it to 48MHz using the USB SOF as a reference */
   XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
  XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

  PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
#endif

  /* Hardware Initialization */
        
  Direct_Init();

  USB_Init();
}


/* Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
  bool ConfigSuccess = true;

  ConfigSuccess &= HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);

  USB_Device_EnableSOFEvents();

//  LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
  HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
  HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 * return Boolean \c true to force the sending of the report, \c false to let the library determine if it needs to be sent
 */

bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
  int i;

  switch (ReportType) {
  case HID_REPORT_ITEM_In:
    {
      USB_KeyboardReport_Data_t* KeyboardReport = (USB_KeyboardReport_Data_t*)ReportData;
      if (NeedEmptyReport) {
        NeedEmptyReport = false;
      }
     
      else {
        AddKeyReport(KeyboardReport);
      }
      *ReportSize = sizeof(USB_KeyboardReport_Data_t);
    }
    return false;
  case HID_REPORT_ITEM_Feature:
    {
      uint8_t* FeatureReport = (uint8_t*)ReportData;
      FeatureReport[0] = (uint8_t)1;
      for (i = 0; i < 1; i++) {
        FeatureReport[i+1] = (uint8_t)CurrentModes[i];
      }
      *ReportSize = 3;
    }
    return true;
  default:
    *ReportSize = 0;
    return false;
  }
}


/* HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */

void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
  int i;

  switch (ReportType) {
  case HID_REPORT_ITEM_Out:
    if (ReportSize > 0) {
#ifdef EXTERNAL_LEDS
      uint8_t* LEDReport = (uint8_t*)ReportData;
      uint8_t  LEDMask   = 0;

      if (*LEDReport & HID_KEYBOARD_LED_NUMLOCK)
        LEDMask |= XLEDS_NUMLOCK;
      if (*LEDReport & HID_KEYBOARD_LED_CAPSLOCK)
        LEDMask |= XLEDS_CAPSLOCK;
      if (*LEDReport & HID_KEYBOARD_LED_SCROLLLOCK)
        LEDMask |= XLEDS_SCROLLLOCK;
      if (*LEDReport & (HID_KEYBOARD_LED_COMPOSE|HID_KEYBOARD_LED_KANA))
        LEDMask |= XLEDS_OTHER;

      XLEDS_PORT = (XLEDS_PORT & ~XLEDS_ALL) | LEDMask;
#endif
      }
      break;
  case HID_REPORT_ITEM_Feature:
    if (ReportSize > 1) {
      uint8_t* FeatureReport = (uint8_t*)ReportData;
      for (i = 0; i < 1; i++) {
        CurrentModes[i] = (TranslationMode)FeatureReport[i+1];
      }
    }
    break;
  }
}
