# QIPIQ Mini V2 экран және сенсор сынағы

Waveshare ESP32-S3-Touch-AMOLED-1.8 V2 платасына арналған. QIPIQ беті жыпылықтайды, ал экранды түрткен сайын Curious, Happy және Surprised күйлері ауысады. Әр ауысу USB Serial журналына жазылады.

Дисплей мен сенсордың GPIO картасы және инициализациясы Waveshare BSP 2.0.3 компонентінен алынады. Жоба BSP-дің ресми quick start мысалындағы инициализация ретін қолданады. [Waveshare quick start](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8/tree/main/examples/esp-idf/00_bsp_quickstart) · [BSP 2.0.3](https://components.espressif.com/components/waveshare/esp32_s3_touch_amoled_1_8/versions/2.0.3/readme)

ESP-IDF 5.5.6 терминалында жоба түбірінен құрастырыңыз. GitHub Actions та осы нұсқамен құрастырып, біріктірілген бинарды артефакт ретінде сақтайды.

```powershell
cd firmware/qipiq_v2
idf.py build
idf.py merge-bin -o qipiq-mini-v2-flash.bin -f raw
```

Жаңа firmware жазар алдында портты және толық 16 МБ сақтық көшірменің барын тексеріңіз. Төмендегі мысалда плата `COM3` портында.

```powershell
python -m esptool --chip esp32s3 --port COM3 flash-id
python -m esptool --chip esp32s3 --port COM3 read-flash 0 ALL qipiq-original.bin
Get-Item qipiq-original.bin | Select-Object Length
Get-FileHash qipiq-original.bin -Algorithm SHA256
```

Көшірменің өлшемі 16 777 216 байт болуы керек. Оны репозиторийден тыс сақтаңыз. Иесі firmware жазуға рұқсат берген соң, жүктеу және журналды қарау үшін ESP-IDF терминалында төмендегі команданы қолданыңыз.

```powershell
idf.py -p COM3 flash monitor
```

Экранда QIPIQ беті мен жыпылықтау көрінуі керек. Үш рет түрткенде журналда `Touch: Happy`, `Touch: Surprised`, `Touch: Curious` жазбалары ретімен шығуы керек. Монитордан шығу үшін `Ctrl+]` басыңыз.

Бастапқы firmware-ді қалпына келтіру қажет болса, дәл осы платадан алынған толық көшірмені көрсетіңіз. Бұл команда флеш жадын қайта жазады.

```powershell
python -m esptool --chip esp32s3 --port COM3 write-flash --flash-mode keep --flash-freq keep --flash-size keep 0x0 qipiq-original.bin
```
