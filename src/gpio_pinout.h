#define PWM_FREQ 2000
#define min_fan_rpm 1200
#define max_fan_rpm 2800
#define fan_rpm_operation_range 1600

enum GPIO_PINOUT{D0 = 16, D1 = 5, D2 = 4, D3 = 0, D4 = 2, D5 = 14, D6 = 12, D7 = 13, D8 = 15};
enum GPIO_STATE{LOW = 0, HIGH = 1};
enum MULTIPLEXER_PINOUT{SELECT0 = D8, SELECT1 = D7, SELECT2 = D6, SELECT3 = D5};
enum DEVICES_PINOUT{led_pin = D1, fan_pin = D2, pump_pin = D3, dht_pin = D5};
