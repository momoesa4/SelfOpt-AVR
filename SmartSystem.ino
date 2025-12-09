// SmartSystem.ino - مثال على استخدام مكتبة الجدولة الذكية
#include <SelfOptScheduler.h>

// إنشاء كائن الجدولة
SelfOptScheduler scheduler(8);

// تعريف المهام
const int SENSOR_PIN = A0;
const int PWM_PIN = 9;
int pwmDuty = 128;

void sensorTask() {
    int v = analogRead(SENSOR_PIN);
    static int filt = 0;
    filt = (filt * 7 + v) / 8;
    // معالجة البيانات...
}

void controlTask() {
    analogWrite(PWM_PIN, pwmDuty);
    // محاكاة حمل معالجة
    volatile int s = 0;
    for (int i = 0; i < 20; i++) s += i * i;
}

void loggerTask() {
    Serial.print("[LOG] ");
    Serial.print(millis());
    Serial.print(" ms | Mem: ");
    Serial.print(scheduler.getFreeMemory());
    Serial.print(" bytes | CPU: ");
    Serial.print(scheduler.getCpuLoad());
    Serial.println("%");
}

void networkTask() {
    // محاكاة مهمة شبكة
    static uint32_t counter = 0;
    counter++;
}

void eventHandler(const char* event, const char* details) {
    Serial.print("[EVENT] ");
    Serial.print(event);
    Serial.print(": ");
    Serial.println(details);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println(F("=== Smart System with Self-Optimizing Scheduler ==="));
    
    // تهيئة الأجهزة
    pinMode(SENSOR_PIN, INPUT);
    pinMode(PWM_PIN, OUTPUT);
    
    // تهيئة الجدولة
    scheduler.begin();
    scheduler.setEventCallback(eventHandler);
    scheduler.enableAdaptiveTuning(true);
    scheduler.enableMemoryProtection(true);
    
    // تسجيل المهام
    scheduler.registerTask("sensor", sensorTask, 100);
    scheduler.registerTask("control", controlTask, 50);
    scheduler.registerTask("logger", loggerTask, 2000);
    scheduler.registerTask("network", networkTask, 500);
    
    // ضبط فاصل طباعة الحالة
    scheduler.setStatusInterval(3000);
    
    Serial.println(F("System ready. Commands: s=status, d=details, t=toggle, r=reset"));
    Serial.println(F("              l=list, m=memory, c=cpu"));
}

void loop() {
    // تشغيل الجدولة
    scheduler.run();
    
    // معالجة أوامر السيريال
    scheduler.handleSerialCommands();
    
    // إضافة تأخير بسيط لتقليل الحمل على المعالج
    delay(1);
}