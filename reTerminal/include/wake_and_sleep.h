
#include <esp_sleep.h>

enum class wakeupState_t {
    Undefined,
    Timer,
    RightButton,
    MidButton,
    LeftButton
};

static wakeupState_t getWakeupState(esp_sleep_wakeup_cause_t cause)
{
    if (cause == ESP_SLEEP_WAKEUP_EXT1) {
        const uint64_t ext1_status = esp_sleep_get_ext1_wakeup_status();
        if (ext1_status & (1ULL << BTN_LEFT_PIN))   return wakeupState_t::LeftButton;
        if (ext1_status & (1ULL << BTN_MIDDLE_PIN)) return wakeupState_t::MidButton;
        if (ext1_status & (1ULL << BTN_RIGHT_PIN))  return wakeupState_t::RightButton;
    } else if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        return wakeupState_t::Timer;
    } else if (cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
        return wakeupState_t::Undefined;
    }
    return wakeupState_t::Timer;
}

static const char *wakeupStateString(wakeupState_t p_wakeupState)
{
    switch (p_wakeupState) {
        case wakeupState_t::Undefined:   return "Undefined";
        case wakeupState_t::Timer:       return "Timer";
        case wakeupState_t::RightButton: return "RightButton";
        case wakeupState_t::MidButton:   return "MidButton";
        case wakeupState_t::LeftButton:  return "LeftButton";
        default:                         return "other";
    }
}

static const char *resetReasonString(esp_reset_reason_t p_ResetReason)
{
    switch (p_ResetReason) {
        case ESP_RST_UNKNOWN:   return "unknown";
        case ESP_RST_POWERON:   return "power on";
        case ESP_RST_EXT:       return "external reset";
        case ESP_RST_SW:        return "software reset";
        case ESP_RST_PANIC:     return "panic";
        case ESP_RST_INT_WDT:   return "interrupt watchdog";
        case ESP_RST_TASK_WDT:  return "task watchdog";
        case ESP_RST_WDT:       return "watchdog";
        case ESP_RST_DEEPSLEEP: return "deep sleep";
        case ESP_RST_BROWNOUT:  return "brownout";
        case ESP_RST_SDIO:      return "sdio";
        default:                return "other";
    }
}

static const char *wakeupReasonString(esp_sleep_wakeup_cause_t p_WeakupReason)
{
    switch (p_WeakupReason) {
    case ESP_SLEEP_WAKEUP_UNDEFINED:        return "ESP_SLEEP_WAKEUP_UNDEFINED";
    case ESP_SLEEP_WAKEUP_ALL:              return "ESP_SLEEP_WAKEUP_ALL";
    case ESP_SLEEP_WAKEUP_EXT0:             return "ESP_SLEEP_WAKEUP_EXT0";
    case ESP_SLEEP_WAKEUP_EXT1:             return "ESP_SLEEP_WAKEUP_EXT1";
    case ESP_SLEEP_WAKEUP_TIMER:            return "ESP_SLEEP_WAKEUP_TIMER";
    case ESP_SLEEP_WAKEUP_TOUCHPAD:         return "ESP_SLEEP_WAKEUP_TOUCHPAD";
    case ESP_SLEEP_WAKEUP_ULP:              return "ESP_SLEEP_WAKEUP_ULP";
    case ESP_SLEEP_WAKEUP_GPIO:             return "ESP_SLEEP_WAKEUP_GPIO";
    case ESP_SLEEP_WAKEUP_UART:             return "ESP_SLEEP_WAKEUP_UART";
    case ESP_SLEEP_WAKEUP_WIFI:             return "ESP_SLEEP_WAKEUP_WIFI";
    case ESP_SLEEP_WAKEUP_COCPU:            return "ESP_SLEEP_WAKEUP_COCPU";
    case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG:  return "ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG";
    case ESP_SLEEP_WAKEUP_BT:               return "ESP_SLEEP_WAKEUP_BT";
    default:                                return "OTHER";
    }
}

static void enterDeepSleep(uint64_t wakeupTimeout)
{
    LOG.println("[SLP] entering deep sleep");
    LOG.flush();

    // Periodic timer wake (microseconds)
    esp_sleep_enable_timer_wakeup(wakeupTimeout * 1000000ULL);

    uint64_t buttonWakeMask = 0;
    for (uint8_t btn : {BTN_LEFT_PIN, BTN_MIDDLE_PIN, BTN_RIGHT_PIN}) {
        buttonWakeMask |= (1ULL << btn);
        // Normal GPIO pull-up is off during deep sleep; use keep-alive domain instead.
        rtc_gpio_pullup_en(static_cast<gpio_num_t>(btn));
        rtc_gpio_pulldown_dis(static_cast<gpio_num_t>(btn));
    }
    esp_sleep_enable_ext1_wakeup(buttonWakeMask, ESP_EXT1_WAKEUP_ANY_LOW);

    esp_deep_sleep_start();
}
