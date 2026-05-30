#!/bin/bash
exePath="./bin/mygit"

echo "========================================"
echo "  СБОРКА ПРОЕКТА"
echo "========================================"
cd ~/mygit
make clean
make
if [ ! -f "$exePath" ]; then
    echo "ОШИБКА СБОРКИ: $exePath не найден"
    exit 1
fi
echo "Сборка успешна: $exePath"
echo ""
echo "========================================"
echo "  ТЕСТЫ MYGIT"
echo "========================================"

rm -rf test_run
mkdir test_run
cd test_run

echo ""
echo "=== 1. Инициализация ==="
$exePath init
$exePath init

echo ""
echo "=== 2. Создание структуры проекта ==="
mkdir -p src/core src/utils include tests
echo "int main() { return 0; }" > main.c
echo "int add(int a, int b) { return a + b; }" > src/core/math.c
echo "int sub(int a, int b) { return a - b; }" > src/core/sub.c
echo "void log(char* msg) {}" > src/utils/logger.c
echo "#define VERSION 1" > include/config.h
echo "#define MAX 100" > include/limits.h
echo "void test_add() {}" > tests/test_math.c
echo "void test_sub() {}" > tests/test_sub.c
echo "Создано файлов:" && find . -type f ! -path "./.mygit/*" | wc -l

echo ""
echo "=== 3. Добавление всех файлов ==="
$exePath add .
$exePath status

echo ""
echo "=== 4. Первый коммит ==="
$exePath commit "Initial project structure"

echo ""
echo "=== 5. Изменение и второй коммит ==="
echo "int add(int a, int b) { return a + b + 1; }" > src/core/math.c
$exePath add src/core/math.c
$exePath commit "Fixed math bug"

echo ""
echo "=== 6. Новая директория и третий коммит ==="
mkdir -p docs
echo "MyGit v1.0" > docs/readme.txt
echo "ChangeLog" > docs/changelog.txt
$exePath add docs
$exePath commit "Added documentation"

echo ""
echo "=== 7. История ==="
$exePath log
HASH1=$($exePath log 2>&1 | grep "commit" | head -3 | tail -1 | awk '{print $2}')
HASH2=$($exePath log 2>&1 | grep "commit" | head -2 | tail -1 | awk '{print $2}')
HASH3=$($exePath log 2>&1 | grep "commit" | head -1 | awk '{print $2}')
echo "HASH1=$HASH1"
echo "HASH2=$HASH2"
echo "HASH3=$HASH3"

echo ""
echo "=== 8. Удаление tests/ ==="
$exePath remove tests/test_math.c
$exePath remove tests/test_sub.c
rm -rf tests
$exePath commit "Removed tests"

echo ""
echo "=== 9. Восстановление из HASH1 ==="
$exePath checkout $HASH1 tests/test_math.c
$exePath checkout $HASH1 tests/test_sub.c
$exePath checkout $HASH1 src/core/math.c
echo "tests/test_math.c:" && cat tests/test_math.c 2>/dev/null || echo "НЕ ВОССТАНОВЛЕН"
echo "src/core/math.c:" && cat src/core/math.c

echo ""
echo "=== 10. Восстановление docs/ ==="
rm -rf docs
$exePath checkout $HASH3 docs/readme.txt
$exePath checkout $HASH3 docs/changelog.txt
ls -la docs/ 2>/dev/null || echo "docs/ не восстановлена"

echo ""
echo "=== 11. diff ==="
$exePath diff $HASH1 $HASH2
echo ""
$exePath diff $HASH1

echo ""
echo "=== 12. Ветки ==="
$exePath branch develop
$exePath branch
$exePath checkout develop
echo "void feature() {}" > feature.c
$exePath add feature.c
$exePath commit "Feature in develop"
$exePath log -n 2
$exePath checkout master
$exePath log -n 2

echo ""
echo "=== 13. Обработка ошибок ==="
$exePath checkout nonexistent
$exePath commit "Empty"
$exePath add nonexistent.txt
$exePath unknown

echo ""
echo "=== 14. log -n ==="
$exePath log -n 2
$exePath log -n 1

echo ""
echo "=== 15. Восстановление удалённого ==="
echo "restore me" > to_delete.txt
$exePath add to_delete.txt
$exePath commit "File to restore"
HASH4=$($exePath log 2>&1 | grep "commit" | head -1 | awk '{print $2}')
rm to_delete.txt
$exePath remove to_delete.txt
$exePath commit "Deleted file"
$exePath checkout $HASH4 to_delete.txt
cat to_delete.txt 2>/dev/null && echo "Восстановлен!" || echo "ОШИБКА!"

echo ""
echo "========================================"
echo "  ТЕСТЫ ЗАВЕРШЕНЫ"
echo "========================================"

cd ~/mygit
rm -rf test_run