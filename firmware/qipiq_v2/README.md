# QIPIQ Mini V2 құрылғы бағдарламасы

Waveshare ESP32-S3-Touch-AMOLED-1.8 V2 платасына арналған.

- Басты бетте QIPIQ беті жыпылықтайды, түрткенде эмоциясын ауыстырады. Ұзақ бассаңыз, сынақ дыбысы шығады.
- **Дауыс жазу (2 с)** микрофон жазбасын жергілікті динамиктен ойнатады.
- **Мәзір → Таймер** 10 секундтан 10 минутқа дейінгі дайын уақыттарды ұсынады. Мәзірге қайтқанда да таймер жүреді.
- **Мәзір → Ойындар** бөлімінде «Сан тап» және «Шапшаңдық» бар.
- **Мәзір → Баптаулар** жарықтықты, дыбысты және қозғалыс реакциясын сақтайды. Батарея мен сенсор күйі осы бетте көрінеді. 30 секунд әрекетсіздіктен кейін экран бәсеңдейді.
- **Мәзір → Wi-Fi → Wi-Fi баптау** басыңыз. Телефонда Espressif ESP SoftAP Provisioning қолданбасын ашып, плата экранындағы QR кодты оқытыңыз. Қолмен баптасаңыз, экрандағы QIPIQ желісін таңдап, көрсетілген кодты уақытша желі құпиясөзі және растау коды ретінде енгізіңіз. Содан кейін қолданбада үйдің 2,4 ГГц желісін таңдаңыз. Үй желісінің деректері құрылғыға сақталады.

Дисплей мен сенсордың GPIO картасы және инициализациясы Waveshare BSP 2.0.3 компонентінен алынады. Жоба BSP-дің ресми quick start мысалындағы инициализация ретін қолданады. [Waveshare quick start](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8/tree/main/examples/esp-idf/00_bsp_quickstart) · [BSP 2.0.3](https://components.espressif.com/components/waveshare/esp32_s3_touch_amoled_1_8/versions/2.0.3/readme)

ESP-IDF 5.5.6 терминалында жоба түбірінен құрастырыңыз. GitHub Actions біріктірілген бинарды және NVS баптауларын сақтай отырып жазуға болатын бөлек файлдарды артефакт ретінде сақтайды.

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

Дайын Windows жинағын жүктесеңіз, ZIP файлын ашып, Windows PowerShell ішінде төмендегі команданы орындаңыз. Скрипт алдымен толық көшірме алады, содан кейін `flash_args` ішіндегі бөлек файлдарды жазады. Дәл осы платадан бұрын алынған толық көшірме болса, оны `-BackupPath` параметрімен беруге болады.

```powershell
python -m pip install esptool
.\Install-QIPIQ.ps1 -Port COM3
```

Экранда QIPIQ беті мен жыпылықтау көрінуі керек. Түрткен сайын журналға `Expression` жазылады. Бетті ұзақ басқанда бір секундтық сынақ дыбысы ойнатылып, журналға `Test tone sent` жазылады.

`Дауыс жазу (2 с)` батырмасын басып, екі секунд сөйлеңіз. Плата 16 кГц жиіліктегі моно жазбаны жедел жадқа алады да, динамикке қайта жібереді. Журнал жазба көлемін және ең жоғары сигнал деңгейін көрсетеді. Жазба ойнатылғаннан кейін жедел жадтан босатылады. Монитордан шығу үшін `Ctrl+]` басыңыз.

Бастапқы firmware-ді қалпына келтіру қажет болса, дәл осы платадан алынған толық көшірмені көрсетіңіз. Бұл команда флеш жадын қайта жазады.

```powershell
python -m esptool --chip esp32s3 --port COM3 write-flash --flash-mode keep --flash-freq keep --flash-size keep 0x0 qipiq-original.bin
```

Қазақша қаріп Noto Sans Regular негізінде LVGL қаріп түрлендіргішімен жасалды. Лицензия осы қалтада. Батарея күйінің регистрлері XPowersLib құжатталған AXP2101 анықтамаларынан алынды. [Noto Sans](https://github.com/notofonts/noto-fonts) · [OFL лицензиясы](OFL-NotoSans.txt) · [XPowersLib](https://github.com/lewisxhe/XPowersLib) · [ESP-IDF Wi-Fi баптауы](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32/api-reference/provisioning/wifi_provisioning.html)
