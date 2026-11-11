# QIPIQ Mini V2 күйі

| Бөлік | Күйі |
| --- | --- |
| 30 эмоция, көз бен ауыз қимылы, жыпылықтау, түрту реакциясы | Implemented and tested |
| Дыбыс сигналы мен ерін қимылының параметрлері | Prototype only |
| ESP-IDF, CO5300, CST820, аудио, IMU, қуат | Not implemented |
| Архивтегі standalone браузер демосы мен FastAPI backend | Prototype only |
| Архивтегі Arduino бұлт сынағы мен STL үлгілері | Prototype only |
| React демосы, firmware ойындары, қазақша AI дауысының құрылғыдағы интеграциясы | Not implemented |
| Бастапқы GitHub Actions тексерулері | Prototype only |
| ZIP релизі, Windows орнату және қалпына келтіру | Not implemented |

Waveshare V2 тақтасында CO5300 (QSPI) және CST820 (I²C) қолданылады. Экран өлшемі 368 × 448. Тақтаның нұсқасын артқы жапсырмасынан тексеруге болады. [Waveshare құжаттамасы](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.8)

GPIO мен инициализацияны тексеру үшін [Waveshare-дің ресми мысалдары](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8) қолданылады. GPIO кестесі мен флештеу командалары әзірге қосылмады.

Жоба түбірінен тестті іске қосу

```powershell
node --test tests/emotions.test.mjs
```
