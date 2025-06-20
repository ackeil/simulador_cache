# GUIA DE USO DO SIMULADOR DE CACHE

## Compilação

```powershell
gcc -o simulador_cache simulador_cache.c -lm
```

## Uso Manual

```powershell
.\simulador_cache.exe
```

O programa irá solicitar os seguintes parâmetros:

1. **Política de escrita**: 0 (Write-through) ou 1 (Write-back)
2. **Tamanho da linha**: Deve ser potência de 2 (ex: 64, 128, 256)
3. **Número de linhas**: Deve ser potência de 2 (ex: 16, 32, 64)
4. **Associatividade**: Número de linhas por conjunto, potência de 2
5. **Tempo de hit**: Tempo de acesso da cache em nanossegundos (ex: 5)
6. **Política de substituição**: 0 (LRU) ou 1 (Aleatória)
7. **Tempo de leitura MP**: Tempo de leitura da memória principal em ns (ex: 70)
8. **Tempo de escrita MP**: Tempo de escrita da memória principal em ns (ex: 70)
9. **Arquivo de entrada**: Nome do arquivo com os endereços (ex: teste.cache)
10. **Arquivo de saída**: Nome do arquivo de resultados (ex: resultado.txt)

## Experimentos Automatizados

### 1. Executar todos os experimentos

```powershell
.\executar_experimentos.ps1
```

Este script executa automaticamente todos os 5 experimentos descritos no README:

- Impacto do Tamanho da Cache
- Impacto do Tamanho do Bloco
- Impacto da Associatividade
- Impacto da Política de Substituição
- Largura de Banda da Memória

### 2. Analisar resultados (requer Python)

```powershell
python analisar_resultados.py
```

Gera gráficos e tabelas automaticamente baseados nos resultados.

**Dependências Python:**

```powershell
pip install matplotlib pandas numpy
```

## Estrutura dos Arquivos de Entrada

Os arquivos `.cache` devem ter o formato:

```
endereço_hex operação
```

Exemplo:

```
0020a858 R
05fea840 W
001947a0 R
```

Onde:

- **endereço_hex**: Endereço de 32 bits em hexadecimal
- **operação**: R (leitura) ou W (escrita)

## Arquivo de Saída

O simulador gera um relatório completo contendo:

- Todos os parâmetros de entrada
- Estatísticas dos acessos (total, leituras, escritas)
- Acessos à memória principal
- Taxas de acerto (hit rates) por tipo e global
- Tempo médio de acesso em nanossegundos

## Exemplo de Uso Rápido

### Teste básico:

```powershell
# Compilar
gcc -o simulador_cache simulador_cache.c -lm

# Executar teste
echo "0`n128`n16`n4`n5`n0`n70`n70`nteste.cache`nresultado.txt" | .\simulador_cache.exe

# Ver resultado
Get-Content resultado.txt
```

### Parâmetros do exemplo:

- Write-through (0)
- Linha de 128 bytes
- 16 linhas totais (2KB de cache)
- 4-way associative (4 conjuntos)
- 5ns de hit time
- LRU (0)
- 70ns para leitura/escrita da MP
- Arquivo: teste.cache
- Saída: resultado.txt

## Interpretação dos Resultados

### Hit Rate

- **Hit rate global**: Percentual de acessos que encontraram o dado na cache
- **Hit rate leitura/escrita**: Percentual específico por tipo de operação

### Tempo Médio de Acesso

Calculado pela fórmula:

```
Tempo_médio = Hit_rate × Hit_time + Miss_rate × (Hit_time + Tempo_MP)
```

### Tráfego de Memória

- **Leituras da MP**: Quantas vezes foi necessário ler da memória principal
- **Escritas na MP**: Quantas vezes foi necessário escrever na memória principal

## Troubleshooting

### Erro de compilação

- Verifique se tem o GCC instalado
- Use `-lm` para linkar a biblioteca matemática

### Parâmetros inválidos

- Tamanhos devem ser potências de 2
- Associatividade não pode ser maior que o número de linhas
- Tempos devem ser positivos

### Arquivo não encontrado

- Verifique se o arquivo de entrada existe no diretório atual
- Use nomes sem espaços ou caracteres especiais
