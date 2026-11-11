# QIPIQ Mini V2 күйі

| Бөлік | Күйі |
| --- | --- |
| 30 эмоция, көз бен ауыз қимылы, жыпылықтау, түрту реакциясы | Implemented; Node unit tests included |
| Browser интерфейсінің standalone демосы | Mock simulation; нақты AI немесе аппараттық байланыс жоқ |
| FastAPI backend (`/health`, қорғалған chat/STT/TTS) | Prototype; unit/smoke tests қосылды, production audit жоқ |
| Backend `/` демо беті | Түзетілді: `web-demo/index.html` файлын береді |
| ESP-IDF, CO5300, CST820, аудио, IMU, қуат | Not implemented / verified емес |
| Arduino cloud сынағы мен STL үлгілері | Prototype only; аппараттық fit сынағы жоқ |
| React демосы, firmware ойындары, құрылғыдағы қазақша AI дауысы | Not implemented |
| GitHub Actions | Python, backend, Node және demo smoke checks қосылды |
| ZIP релизі, Windows орнату және қалпына келтіру | Not implemented |

## Аппараттық шектеулер

Waveshare V2 тақшасында CO5300 (QSPI) және CST820 (I²C) қолданылады; экран өлшемі 368 × 448. GPIO, аудио және инициализация мәндері нақты тақша ревизиясының [Waveshare ресми мысалдарымен](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8) тексерілмейінше firmware-ге енгізілмеуі керек. Нақты құрылғыда build/flash тесті орындалған жоқ.

## Жергілікті тексеру

```sh
python -m pip install -r backend/requirements.txt
python -m compileall -q backend
python -m unittest discover -s tests -p 'test_*.py' -v
node --test tests/*.test.mjs
```

GitHub Actions осы тексерулерді PR және push кезінде іске қосады. Бұл тесттердің конфигурация мен кейбір жолдарын тексеретінін, live OpenAI/микрофон/құрылғы сынағы емес екенін ескеріңіз.
