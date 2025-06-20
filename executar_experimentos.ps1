# Script PowerShell para executar os experimentos de análise de cache
# Baseado nas especificações do README.md

Write-Host "=== SIMULADOR DE CACHE - EXPERIMENTOS AUTOMATIZADOS ===" -ForegroundColor Green
Write-Host "Este script executa todos os experimentos descritos no README.md" -ForegroundColor Yellow
Write-Host ""

# Função para executar simulação
function Executar-Simulacao {
    param(
        [int]$politica_escrita,
        [int]$tamanho_linha,
        [int]$numero_linhas,
        [int]$associatividade,
        [int]$hit_time,
        [int]$politica_subs,
        [int]$tempo_mp_leitura,
        [int]$tempo_mp_escrita,
        [string]$arquivo_entrada,
        [string]$arquivo_saida
    )
    
    $input_data = @"
$politica_escrita
$tamanho_linha
$numero_linhas
$associatividade
$hit_time
$politica_subs
$tempo_mp_leitura
$tempo_mp_escrita
$arquivo_entrada
$arquivo_saida
"@
    
    $input_data | .\simulador_cache.exe
}

# Verificar se o executável existe
if (-not (Test-Path "simulador_cache.exe")) {
    Write-Host "Compilando simulador..." -ForegroundColor Yellow
    gcc -o simulador_cache simulador_cache.c -lm
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Erro na compilação!" -ForegroundColor Red
        exit 1
    }
}

# Criar diretório para resultados
if (-not (Test-Path "resultados")) {
    New-Item -ItemType Directory -Name "resultados"
}

Write-Host "=== EXPERIMENTO 1: Impacto do Tamanho da Cache ===" -ForegroundColor Cyan
Write-Host "Bloco: 128 bytes, Write-through, LRU, Associatividade: 4" -ForegroundColor Yellow

# Experimento 1: Variando número de blocos (8, 16, 32, 64, 128, 256, 512, 1024)
$blocos = @(8, 16, 32, 64, 128, 256, 512, 1024)
foreach ($num_blocos in $blocos) {
    $tamanho_cache = $num_blocos * 128
    Write-Host "Executando: $num_blocos blocos (${tamanho_cache} bytes)..."
    Executar-Simulacao -politica_escrita 0 -tamanho_linha 128 -numero_linhas $num_blocos -associatividade 4 -hit_time 5 -politica_subs 0 -tempo_mp_leitura 70 -tempo_mp_escrita 70 -arquivo_entrada "oficial.cache" -arquivo_saida "resultados/exp1_tamanho_${num_blocos}blocos.txt"
}

Write-Host "=== EXPERIMENTO 2: Impacto do Tamanho do Bloco ===" -ForegroundColor Cyan
Write-Host "Cache: 8KB, Write-through, LRU, Associatividade: 2" -ForegroundColor Yellow

# Experimento 2: Variando tamanho do bloco (8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096)
$tamanhos_bloco = @(8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096)
foreach ($tam_bloco in $tamanhos_bloco) {
    $num_linhas = 8192 / $tam_bloco  # 8KB / tamanho do bloco
    if ($num_linhas -ge 2) {  # Verificar se é possível ter associatividade 2
        Write-Host "Executando: Bloco de $tam_bloco bytes ($num_linhas linhas)..."
        Executar-Simulacao -politica_escrita 0 -tamanho_linha $tam_bloco -numero_linhas $num_linhas -associatividade 2 -hit_time 5 -politica_subs 0 -tempo_mp_leitura 70 -tempo_mp_escrita 70 -arquivo_entrada "oficial.cache" -arquivo_saida "resultados/exp2_bloco_${tam_bloco}bytes.txt"
    }
}

Write-Host "=== EXPERIMENTO 3: Impacto da Associatividade ===" -ForegroundColor Cyan
Write-Host "Bloco: 128 bytes, Write-back, LRU, Cache: 8KB" -ForegroundColor Yellow

# Experimento 3: Variando associatividade (1, 2, 4, 8, 16, 32, 64)
$associatividades = @(1, 2, 4, 8, 16, 32, 64)
$num_linhas_total = 8192 / 128  # 8KB / 128 bytes = 64 linhas
foreach ($assoc in $associatividades) {
    if ($assoc -le $num_linhas_total) {
        Write-Host "Executando: Associatividade $assoc..."
        Executar-Simulacao -politica_escrita 1 -tamanho_linha 128 -numero_linhas $num_linhas_total -associatividade $assoc -hit_time 5 -politica_subs 0 -tempo_mp_leitura 70 -tempo_mp_escrita 70 -arquivo_entrada "oficial.cache" -arquivo_saida "resultados/exp3_assoc_${assoc}.txt"
    }
}

Write-Host "=== EXPERIMENTO 4: Impacto da Política de Substituição ===" -ForegroundColor Cyan
Write-Host "Bloco: 128 bytes, Write-through, Associatividade: 4" -ForegroundColor Yellow

# Experimento 4: Comparando LRU vs Aleatório
$blocos_exp4 = @(16, 32, 64, 128, 256, 512, 1024)
foreach ($num_blocos in $blocos_exp4) {
    $tamanho_cache = $num_blocos * 128
    
    # LRU
    Write-Host "Executando: $num_blocos blocos - LRU..."
    Executar-Simulacao -politica_escrita 0 -tamanho_linha 128 -numero_linhas $num_blocos -associatividade 4 -hit_time 5 -politica_subs 0 -tempo_mp_leitura 70 -tempo_mp_escrita 70 -arquivo_entrada "oficial.cache" -arquivo_saida "resultados/exp4_${num_blocos}blocos_LRU.txt"
    
    # Aleatório
    Write-Host "Executando: $num_blocos blocos - Aleatório..."
    Executar-Simulacao -politica_escrita 0 -tamanho_linha 128 -numero_linhas $num_blocos -associatividade 4 -hit_time 5 -politica_subs 1 -tempo_mp_leitura 70 -tempo_mp_escrita 70 -arquivo_entrada "oficial.cache" -arquivo_saida "resultados/exp4_${num_blocos}blocos_ALEATORIO.txt"
}

Write-Host "=== EXPERIMENTO 5: Largura de Banda da Memória ===" -ForegroundColor Cyan
Write-Host "Comparando Write-through vs Write-back" -ForegroundColor Yellow

# Experimento 5: Largura de banda - comparando políticas de escrita
$configuracoes = @(
    @{cache_kb=8; bloco=64; assoc=2},
    @{cache_kb=8; bloco=64; assoc=4},
    @{cache_kb=8; bloco=128; assoc=2},
    @{cache_kb=8; bloco=128; assoc=4},
    @{cache_kb=16; bloco=64; assoc=2},
    @{cache_kb=16; bloco=64; assoc=4},
    @{cache_kb=16; bloco=128; assoc=2},
    @{cache_kb=16; bloco=128; assoc=4}
)

foreach ($config in $configuracoes) {
    $num_linhas = ($config.cache_kb * 1024) / $config.bloco
    
    # Write-through
    Write-Host "Executando: ${config.cache_kb}KB, ${config.bloco}B, ${config.assoc}-way - Write-through..."
    Executar-Simulacao -politica_escrita 0 -tamanho_linha $config.bloco -numero_linhas $num_linhas -associatividade $config.assoc -hit_time 5 -politica_subs 0 -tempo_mp_leitura 70 -tempo_mp_escrita 70 -arquivo_entrada "oficial.cache" -arquivo_saida "resultados/exp5_${config.cache_kb}KB_${config.bloco}B_${config.assoc}way_WT.txt"
    
    # Write-back
    Write-Host "Executando: ${config.cache_kb}KB, ${config.bloco}B, ${config.assoc}-way - Write-back..."
    Executar-Simulacao -politica_escrita 1 -tamanho_linha $config.bloco -numero_linhas $num_linhas -associatividade $config.assoc -hit_time 5 -politica_subs 0 -tempo_mp_leitura 70 -tempo_mp_escrita 70 -arquivo_entrada "oficial.cache" -arquivo_saida "resultados/exp5_${config.cache_kb}KB_${config.bloco}B_${config.assoc}way_WB.txt"
}

Write-Host ""
Write-Host "=== TODOS OS EXPERIMENTOS CONCLUÍDOS ===" -ForegroundColor Green
Write-Host "Os resultados estão na pasta 'resultados/'" -ForegroundColor Yellow
Write-Host "Use os arquivos de saída para criar os gráficos e análises solicitadas." -ForegroundColor Yellow
