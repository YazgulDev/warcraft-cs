# Установка Warcraft CS 0.1.0

Нужны собственные Warcraft III **1.26a x86** и Counter-Strike 1.6.
Проект не скачивает игры, не поставляет их контент и не изменяет оригинальную установку.

1. Установите Git for Windows, Python 3.10+ с доступным `python`, Visual Studio 2022
   или Build Tools: workload **Desktop development with C++**, MSVC x86/x64 и Windows SDK.
2. Клонируйте `https://github.com/whitestridee/warcraft-cs.git` в отдельную папку,
   например `C:\Warcraft-CS`, и откройте PowerShell в ней.
3. Убедитесь, что `Game.dll` в вашей игре имеет версию **1.26.0.6401**.
   Reforged и другие патчи не поддерживаются. Требуются также оригинальные `Mss32.dll`,
   `Storm.dll`, `war3.exe` и четыре MPQ Warcraft/TFT.
4. Найдите `cstrike` внутри своей установки CS. В ней должны присутствовать `models/v_ak47.mdl`,
   `v_m4a1.mdl`, `v_usp.mdl`, `v_awp.mdl`, `v_knife.mdl`, `v_c4.mdl` и `sound/weapons`, `sound/player`.
   Путь у Steam часто заканчивается на `steamapps/common/Half-Life/cstrike`.
   Setup ожидает доступные локальные модели/звуки и не загружает чужие игровые архивы.
5. Сохраните текущую игру и закройте Warcraft. Выполните, заменив пути:

```powershell
.\setup.cmd -WarcraftDirectory "E:\Warcraft III" -CounterStrikeDirectory "C:\SteamGames\steamapps\common\Half-Life\cstrike"
```

Setup создаёт `.local/warcraft-cs`, копирует туда нужные файлы из вашей игры, сохраняет
оригинальный звуковой модуль как `WarcraftOriginalMss.dll`, конвертирует ваши CS-материалы,
собирает `WarcraftCS.mix` и собственный прокси `Mss32.dll`. Экспортные имена/ординалы
вычисляются локально из вашей DLL. В репозиторий или GitHub эти файлы не попадают.
MinHook v1.3.4 загружается по закреплённой ревизии с SHA256-проверкой в `.local/dependencies`;
NumPy устанавливается в `.local/venv`. Machine-specific пути сохраняются в `.local/setup.json`.

Если Python не на PATH, вызовите PowerShell-скрипт напрямую с `-PythonExecutable`:

```powershell
.\setup\setup.ps1 -WarcraftDirectory "E:\Warcraft III" -CounterStrikeDirectory "C:\SteamGames\steamapps\common\Half-Life\cstrike" -PythonExecutable "C:\Python312\python.exe"
```

Скин из `grudge_and_poison_sword.zip` не распространяется. Если у вас есть право пользоваться
своей копией, распакуйте её локально и добавьте
`-SwordModel "C:\PrivateModels\v_grudge_sword.mdl"`. Импортируются его скин, кости и анимации.
Без аргумента используется процедурный меч с руками/анимацией из вашей модели ножа CS.
Права на стороннюю модель, её происхождение и разрешения определяются отдельно от лицензии кода.

Запуск:

```powershell
.\play.cmd
.\play.cmd -Windowed
.\play.cmd -Map "E:\Warcraft III\Maps\(4)LostTemple.w3m"
```

По умолчанию — полноэкранное меню Warcraft. Выберите одиночную карту/кампанию,
своего живого юнита и F6. Пауза — F10. Любые тестовые сцены запускайте только в отдельном сражении.

Пересборка после изменений (закройте Warcraft): `./tools/build.ps1`.
Тесты без игровых данных: `./tools/test-all.ps1`.
INI сохраняется при пересборке. Повторный setup обновляет только созданную им приватную копию;
папки без маркера setup не перезаписывает. Исходные сохранения автоматически не копируются.
При необходимости перенесите свои сохранения вручную из `save`, сохранив резервную копию.

Удаление: закройте Warcraft, сохраните нужный прогресс из приватного `save`, затем удалите
свою папку проекта/приватную копию обычными средствами Windows. Оригинальные игры останутся.
Не публикуйте содержимое `.local`, конвертированные материалы или сборки с игровыми DLL.
