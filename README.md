# KwasOS
Реальная операционнная система,  
которую я решил посветить любимому напитку - квасу.  
Пока что эта система ещё маленькая,  
но постепенно она будет становиться лучше!  
## Оглавление
* [Команды](#команды)  
* [Общие требования](#общие-требования)  
* [Как установить](#как-установить)  
1. [Linux (Ubuntu / Debian / Fedora / Arch)](#linux-ubuntu--debian--fedora--arch)  
2. [MacOS](#macos)  
3. [Windows (wsl2)](#windows-через-wsl2--рекомендуется)
4. [Windows (Cygwin или MSYS2)](#windows-cygwin-или-msys2--без-wsl)
5. [Docker](#альтернатива--docker)
* [Последние изменения](#последние-изменения)  
* [ВАЖНО](#важно)  
## Команды
help         - Показать все команды,    
echo <Текст> - Вывести текст,  
ver          - Показать версию системы,  
cls          - Очистить экран,  
reboot       - Перезагрузить систему,  
shutdown     - Выключить компьютер.
## Общие требования
* Git – для клонирования репозитория.  
* NASM – для сборки загрузчика.  
* Кросс-компилятор `x86_64-elf-gcc` – для компиляции ядра.  
* QEMU – для эмуляции (рекомендуется `qemu-system-x86_64`).  
* Xorriso – для создания ISO-образа (входит в пакет `libisoburn` или `xorriso`).  
## Как установить
### Linux (Ubuntu / Debian / Fedora / Arch)
#### Установка зависимостей
* Ubuntu / Debian:
```
sudo apt update
sudo apt install -y git nasm qemu-system-x86 xorriso build-essential
sudo apt install -y gcc-multilib g++-multilib   # если нужны 32-битные библиотеки
# Для кросс-компилятора x86_64-elf:
sudo apt install -y gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu
# Или можно установить кросс-компилятор из пакета gcc-12-x86-64-linux-gnu (зависит от версии)
```
* Fedora / RHEL:
```
sudo dnf install -y git nasm qemu-system-x86 xorriso gcc make
sudo dnf install -y binutils
# Для кросс-компиляции x86_64-elf можно установить пакет gcc-x86_64-linux-gnu
```
* Arch Linux:
```
sudo pacman -S git nasm qemu-desktop xorriso gcc make
# Для кросс-компилятора:
sudo pacman -S x86_64-elf-gcc
```
Если пакет x86_64-elf-gcc недоступен в вашем дистрибутиве,  
можно установить его вручную или использовать системный gcc с флагом -m64  
(он будет генерировать код для вашей архитектуры, но для ядра лучше использовать кросс-компилятор).  
Рекомендуется установить кросс-компилятор, чтобы избежать проблем с линковкой и форматом ELF.  
Для простоты на Debian/Ubuntu можно использовать gcc и ld из пакета build-essential – они умеют собирать 64-битные ELF-файлы,  
если указать -m64 и -ffreestanding. Однако в вашем makefile уже используется x86_64-elf-gcc, поэтому установите его.
Альтернатива – изменить makefile на использование системного gcc  
(он будет генерировать код для x86-64, но может потребоваться установить gcc-multilib). Я рекомендую оставить x86_64-elf-gcc и установить его.  
#### Клонирование и сборка
```
git clone https://github.com/frendalkir-alt/KwasOS.git
cd KwasOS
make
make run
```
### MacOS
#### Установка Homebrew (если ещё нет)
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```
#### Установка зависимостей
```
brew install git nasm qemu xorriso
brew install x86_64-elf-gcc   # кросс-компилятор
```
Если `x86_64-elf-gcc` не установился, попробуйте:  
```
brew tap nativeos/i386-elf-toolchain
brew install i386-elf-gcc   # он содержит x86_64-elf-gcc?
```
Или установите через пакет `gcc` и используйте системный компилятор (но тогда измените `CC` в makefile на `gcc`). Однако у вас уже есть `x86_64-elf-gcc`, так что проверьте:  
```
which x86_64-elf-gcc
```
Если нет, попробуйте `brew install x86_64-elf-binutils` и `brew install x86_64-elf-gcc` из стороннего тапа.  
#### Клонирование и сборка
```
git clone https://github.com/frendalkir-alt/KwasOS.git
cd KwasOS
make
make run
```
### Windows (через WSL2 – рекомендуется)
#### Установите WSL2 и дистрибутив (Ubuntu)
Откройте PowerShell от имени администратора:  
```
wsl --install
```
Перезагрузитесь, если потребуется, и запустите Ubuntu из меню Пуск.  
#### Внутри Ubuntu выполните команды для Linux (см. раздел Ubuntu/Debian).
После установки зависимостей:  
```
git clone https://github.com/frendalkir-alt/KwasOS.git
cd KwasOS
make
make run
```
QEMU будет работать в графическом режиме,  
если у вас установлен X-сервер (например, VcXsrv) – настройте DISPLAY.  
Или используйте `-nographic` для запуска в консоли.  
### Windows (Cygwin или MSYS2 – без WSL)
Это сложнее, но возможно. Установите:  
Cygwin или MSYS2.  
Пакеты: `gcc`, `nasm`, `make`, `qemu`, `xorriso` (возможно, придётся собирать из исходников).  
Кросс-компилятор `x86_64-elf-gcc` для Windows можно найти в виде готовых сборок  
(например, на сайте GNU или через MSYS2).  
Затем действуйте аналогично.  
### Альтернатива – Docker
Если вы не хотите устанавливать всё локально, можно использовать Docker-образ с уже  
установленными инструментами. Например, создайте Dockerfile:  
```
FROM ubuntu:22.04
RUN apt update && apt install -y git nasm qemu-system-x86 xorriso build-essential gcc-x86-64-linux-gnu
```
Затем соберите и запустите внутри контейнера.  
## Последние изменения
1. Создана файловая система
2. Создан таймер тиков и секунд
3. Добавлен аналог BSOD
4. Обновлены драйвера
## ВАЖНО
Пока что я не тестировал систему  
на реальном компьютере, (не в qemu)  
и по этому не рекомендую запускать  
её на реальном компьютере.
