@echo off
setlocal enabledelayedexpansion

REM Compile before running tests
gcc simulador_cache.c -o simulador_cache.exe
if errorlevel 1 (
    echo Compilation failed!
    exit /b 1
)

REM Ensure output directory exists
if not exist ".\saidas\oficial" mkdir ".\saidas\oficial"

REM Experiment 1: Impacto do Tamanho da Cache
if not exist ".\saidas\oficial\impacto_tamanho_cache" mkdir ".\saidas\oficial\impacto_tamanho_cache"
for %%i in (8 16 32 64 128) do (
    (
        echo 0
        echo 128
        echo %%i
        echo 4
        echo 5
        echo 0
        echo 70
        echo 70
        echo 1
        echo .\saidas\oficial\impacto_tamanho_cache\impacto_tamanho_cache_%%i.csv
    ) > temp_input.txt
    .\simulador_cache.exe < temp_input.txt
    del temp_input.txt
)

REM Experiment 2: Impacto do Tamanho do Bloco
if not exist ".\saidas\oficial\impacto_tamanho_bloco" mkdir ".\saidas\oficial\impacto_tamanho_bloco"
for %%i in (8 16 32 64 128 256 512 1024 2048 4096) do (
    (
        echo 0
        echo %%i
        echo 128
        echo 8
        echo 5
        echo 0
        echo 70
        echo 70
        echo 1
        echo .\saidas\oficial\impacto_tamanho_bloco\impacto_tamanho_bloco_%%i.csv
    ) > temp_input.txt
    .\simulador_cache.exe < temp_input.txt
    del temp_input.txt
)

REM Experiment 3: Impacto da Associatividade
if not exist ".\saidas\oficial\impacto_associatividade" mkdir ".\saidas\oficial\impacto_associatividade"
for %%i in (2 4 8 16 32 64) do (
    (
        echo 1
        echo 128
        echo 128
        echo %%i
        echo 5
        echo 0
        echo 70
        echo 70
        echo 1
        echo .\saidas\oficial\impacto_associatividade\impacto_associatividade_%%i.csv
    ) > temp_input.txt
    .\simulador_cache.exe < temp_input.txt
    del temp_input.txt
)

REM Experiment 4: Impacto da Política de Substituição
if not exist ".\saidas\oficial\impacto_politica_substituicao" mkdir ".\saidas\oficial\impacto_politica_substituicao"
for %%i in (16 32 64 128) do (
    for %%p in (0 1) do (
        (
            echo 0
            echo 128
            echo %%i
            echo 4
            echo 5
            echo %%p
            echo 70
            echo 70
            echo 1
            echo .\saidas\oficial\impacto_politica_substituicao\impacto_politica_substituicao_%%p_%%i.csv
        ) > temp_input.txt
        .\simulador_cache.exe < temp_input.txt
        del temp_input.txt
    )
)

REM Experiment 5: Largura de Banda da Memória
if not exist ".\saidas\oficial\largura_banda_memoria" mkdir ".\saidas\oficial\largura_banda_memoria"
for %%s in (8 16) do (
    for %%b in (64 128) do (
        for %%a in (2 4) do (
            set /a size=%%s*1024
            (
                echo 0
                echo %%b
                echo !size!
                echo %%a
                echo 5
                echo 0
                echo 70
                echo 70
                echo 1
                echo .\saidas\oficial\largura_banda_memoria\largura_banda_memoria_%%sKb_%%bB_%%a.csv
            ) > temp_input.txt
            .\simulador_cache.exe < temp_input.txt
            del temp_input.txt
        )
    )
)

endlocal