$exePath = "C:\Projects\mygit\x64\Debug\mygit.exe"
function mygit { & $exePath $args }

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  СБОРКА ПРОЕКТА" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Set-Location C:\Projects\mygit\mygit
make clean 2>&1 | Out-Null
make 2>&1 | Out-Null
if (Test-Path "bin\mygit.exe") {
    Write-Host "Сборка успешна: bin\mygit.exe" -ForegroundColor Green
} else {
    Write-Host "ОШИБКА СБОРКИ!" -ForegroundColor Red
    exit 1
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  ТЕСТЫ MYGIT" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

Remove-Item test_run -Recurse -Force -ErrorAction SilentlyContinue
mkdir test_run
Set-Location test_run

Write-Host "`n=== 1. Инициализация ==="
mygit init
mygit init

Write-Host "`n=== 2. Создание структуры ==="
mkdir src\core, src\utils, include, tests -Force
Set-Content -Path main.c -Value "int main() { return 0; }"
Set-Content -Path src\core\math.c -Value "int add(int a, int b) { return a + b; }"
Set-Content -Path src\core\sub.c -Value "int sub(int a, int b) { return a - b; }"
Set-Content -Path src\utils\logger.c -Value "void log(char* msg) {}"
Set-Content -Path include\config.h -Value "#define VERSION 1"
Set-Content -Path include\limits.h -Value "#define MAX 100"
Set-Content -Path tests\test_math.c -Value "void test_add() {}"
Set-Content -Path tests\test_sub.c -Value "void test_sub() {}"

Write-Host "`n=== 3. mygit add . ==="
mygit add .
mygit status

Write-Host "`n=== 4. Первый коммит ==="
mygit commit "Initial project structure"

Write-Host "`n=== 5. Изменение и коммит ==="
Set-Content -Path src\core\math.c -Value "int add(int a, int b) { return a + b + 1; }"
mygit add src\core\math.c
mygit commit "Fixed math bug"

Write-Host "`n=== 6. Новая директория ==="
mkdir docs -Force
Set-Content -Path docs\readme.txt -Value "MyGit v1.0"
Set-Content -Path docs\changelog.txt -Value "ChangeLog"
mygit add docs
mygit commit "Added documentation"

Write-Host "`n=== 7. История ==="
mygit log
$log = mygit log
$hashes = ($log | Select-String "commit " | ForEach-Object { $_.ToString().Replace("commit ", "").Trim() })
$HASH1 = $hashes[2]
$HASH2 = $hashes[1]
$HASH3 = $hashes[0]

Write-Host "`n=== 8. Удаление tests/ ==="
mygit remove tests/test_math.c
mygit remove tests/test_sub.c
Remove-Item tests -Recurse -Force
mygit commit "Removed tests"

Write-Host "`n=== 9. Восстановление из HASH1 ==="
mygit checkout $HASH1 tests/test_math.c
mygit checkout $HASH1 tests/test_sub.c
mygit checkout $HASH1 src/core/math.c
Write-Host "tests/test_math.c:"; if (Test-Path tests\test_math.c) { Get-Content tests\test_math.c } else { Write-Host "НЕ ВОССТАНОВЛЕН" }
Write-Host "src/core/math.c:"; if (Test-Path src\core\math.c) { Get-Content src\core\math.c } else { Write-Host "НЕ ВОССТАНОВЛЕН" }

Write-Host "`n=== 10. Восстановление docs/ ==="
Remove-Item docs -Recurse -Force -ErrorAction SilentlyContinue
mygit checkout $HASH3 docs/readme.txt
mygit checkout $HASH3 docs/changelog.txt

Write-Host "`n=== 11. diff ==="
mygit diff $HASH1 $HASH2
mygit diff $HASH1

Write-Host "`n=== 12. Ветки ==="
mygit branch develop
mygit branch
mygit checkout develop
Set-Content -Path feature.c -Value "void feature() {}"
mygit add feature.c
mygit commit "Feature in develop"
mygit log -n 2
mygit checkout master
mygit log -n 2

Write-Host "`n=== 13. Ошибки ==="
mygit checkout nonexistent
mygit commit "Empty"
mygit add nonexistent.txt
mygit unknown

Write-Host "`n=== 14. log -n ==="
mygit log -n 2
mygit log -n 1

Write-Host "`n=== 15. Восстановление удалённого ==="
Set-Content -Path to_delete.txt -Value "restore me"
mygit add to_delete.txt
mygit commit "File to restore"
$log = mygit log
$HASH4 = ($log | Select-String "commit " | Select-Object -First 1).ToString().Replace("commit ", "").Trim()
Remove-Item to_delete.txt -Force
mygit remove to_delete.txt
mygit commit "Deleted file"
mygit checkout $HASH4 to_delete.txt
if (Test-Path to_delete.txt) { Get-Content to_delete.txt; Write-Host "Восстановлен!" } else { Write-Host "ОШИБКА!" }

Write-Host "`n========================================" -ForegroundColor Green
Write-Host "  ТЕСТЫ ЗАВЕРШЕНЫ" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

Set-Location C:\Projects\mygit\mygit
Remove-Item test_run -Recurse -Force -ErrorAction SilentlyContinue