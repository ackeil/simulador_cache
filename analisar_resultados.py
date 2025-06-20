#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Script para análise dos resultados do simulador de cache
Gera gráficos e tabelas conforme especificado no README.md
"""

import os
import re
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from pathlib import Path

def extrair_dados_arquivo(caminho_arquivo):
    """Extrai dados de um arquivo de resultado do simulador"""
    dados = {}
    
    try:
        with open(caminho_arquivo, 'r', encoding='utf-8') as f:
            conteudo = f.read()
        
        # Extrair parâmetros
        dados['politica_escrita'] = re.search(r'Política de escrita: (.+)', conteudo).group(1)
        dados['tamanho_linha'] = int(re.search(r'Tamanho da linha: (\d+) bytes', conteudo).group(1))
        dados['numero_linhas'] = int(re.search(r'Número de linhas: (\d+)', conteudo).group(1))
        dados['associatividade'] = int(re.search(r'Associatividade: (\d+) linhas', conteudo).group(1))
        dados['politica_substituicao'] = re.search(r'Política de substituição: (.+)', conteudo).group(1)
        
        # Extrair estatísticas
        dados['total_acessos'] = int(re.search(r'Total de acessos: (\d+)', conteudo).group(1))
        dados['total_leituras'] = int(re.search(r'Total de leituras: (\d+)', conteudo).group(1))
        dados['total_escritas'] = int(re.search(r'Total de escritas: (\d+)', conteudo).group(1))
        dados['leituras_mp'] = int(re.search(r'Leituras da MP: (\d+)', conteudo).group(1))
        dados['escritas_mp'] = int(re.search(r'Escritas na MP: (\d+)', conteudo).group(1))
        
        # Extrair hit rates
        hit_rate_global = re.search(r'Hit rate global: ([\d.]+)%', conteudo).group(1)
        dados['hit_rate_global'] = float(hit_rate_global)
        
        # Calcular tamanho da cache
        dados['tamanho_cache'] = dados['numero_linhas'] * dados['tamanho_linha']
        
        # Tempo médio
        tempo_medio = re.search(r'Tempo médio de acesso: ([\d.]+) ns', conteudo).group(1)
        dados['tempo_medio'] = float(tempo_medio)
        
    except Exception as e:
        print(f"Erro ao processar {caminho_arquivo}: {e}")
        return None
    
    return dados

def gerar_grafico_impacto_tamanho_cache():
    """Gráfico 1: Impacto do Tamanho da Cache"""
    dados = []
    
    # Buscar arquivos do experimento 1
    for arquivo in Path('resultados').glob('exp1_tamanho_*blocos.txt'):
        dados_arquivo = extrair_dados_arquivo(arquivo)
        if dados_arquivo:
            dados.append(dados_arquivo)
    
    if not dados:
        print("Nenhum dado encontrado para experimento 1")
        return
    
    # Ordenar por tamanho da cache
    dados.sort(key=lambda x: x['tamanho_cache'])
    
    tamanhos = [d['tamanho_cache'] for d in dados]
    hit_rates = [d['hit_rate_global'] for d in dados]
    
    plt.figure(figsize=(10, 6))
    plt.plot(tamanhos, hit_rates, 'bo-', linewidth=2, markersize=8)
    plt.xlabel('Tamanho da Cache (bytes)')
    plt.ylabel('Taxa de Acerto (%)')
    plt.title('Impacto do Tamanho da Cache na Taxa de Acerto')
    plt.grid(True, alpha=0.3)
    plt.xscale('log', base=2)
    
    # Adicionar rótulos nos pontos
    for i, (x, y) in enumerate(zip(tamanhos, hit_rates)):
        plt.annotate(f'{y:.1f}%', (x, y), textcoords="offset points", xytext=(0,10), ha='center')
    
    plt.tight_layout()
    plt.savefig('graficos/impacto_tamanho_cache.png', dpi=300, bbox_inches='tight')
    plt.show()

def gerar_grafico_impacto_tamanho_bloco():
    """Gráfico 2: Impacto do Tamanho do Bloco"""
    dados = []
    
    # Buscar arquivos do experimento 2
    for arquivo in Path('resultados').glob('exp2_bloco_*bytes.txt'):
        dados_arquivo = extrair_dados_arquivo(arquivo)
        if dados_arquivo:
            dados.append(dados_arquivo)
    
    if not dados:
        print("Nenhum dado encontrado para experimento 2")
        return
    
    # Ordenar por tamanho do bloco
    dados.sort(key=lambda x: x['tamanho_linha'])
    
    tamanhos_bloco = [d['tamanho_linha'] for d in dados]
    hit_rates = [d['hit_rate_global'] for d in dados]
    
    plt.figure(figsize=(10, 6))
    plt.plot(tamanhos_bloco, hit_rates, 'ro-', linewidth=2, markersize=8)
    plt.xlabel('Tamanho do Bloco (bytes)')
    plt.ylabel('Taxa de Acerto (%)')
    plt.title('Impacto do Tamanho do Bloco na Taxa de Acerto')
    plt.grid(True, alpha=0.3)
    plt.xscale('log', base=2)
    
    # Encontrar tamanho ótimo
    idx_otimo = np.argmax(hit_rates)
    tamanho_otimo = tamanhos_bloco[idx_otimo]
    hit_rate_otimo = hit_rates[idx_otimo]
    
    plt.annotate(f'Ótimo: {tamanho_otimo}B\\n{hit_rate_otimo:.1f}%', 
                xy=(tamanho_otimo, hit_rate_otimo),
                xytext=(tamanho_otimo*2, hit_rate_otimo+2),
                arrowprops=dict(arrowstyle='->', color='red'),
                fontsize=12, color='red', weight='bold')
    
    plt.tight_layout()
    plt.savefig('graficos/impacto_tamanho_bloco.png', dpi=300, bbox_inches='tight')
    plt.show()
    
    print(f"Tamanho de bloco ótimo: {tamanho_otimo} bytes com {hit_rate_otimo:.1f}% de hit rate")

def gerar_grafico_impacto_associatividade():
    """Gráfico 3: Impacto da Associatividade"""
    dados = []
    
    # Buscar arquivos do experimento 3
    for arquivo in Path('resultados').glob('exp3_assoc_*.txt'):
        dados_arquivo = extrair_dados_arquivo(arquivo)
        if dados_arquivo:
            dados.append(dados_arquivo)
    
    if not dados:
        print("Nenhum dado encontrado para experimento 3")
        return
    
    # Ordenar por associatividade
    dados.sort(key=lambda x: x['associatividade'])
    
    associatividades = [d['associatividade'] for d in dados]
    hit_rates = [d['hit_rate_global'] for d in dados]
    
    plt.figure(figsize=(10, 6))
    plt.plot(associatividades, hit_rates, 'go-', linewidth=2, markersize=8)
    plt.xlabel('Associatividade (vias)')
    plt.ylabel('Taxa de Acerto (%)')
    plt.title('Impacto da Associatividade na Taxa de Acerto')
    plt.grid(True, alpha=0.3)
    plt.xscale('log', base=2)
    
    plt.tight_layout()
    plt.savefig('graficos/impacto_associatividade.png', dpi=300, bbox_inches='tight')
    plt.show()

def gerar_grafico_impacto_politica_substituicao():
    """Gráfico 4: Impacto da Política de Substituição"""
    dados_lru = []
    dados_aleatorio = []
    
    # Buscar arquivos do experimento 4
    for arquivo in Path('resultados').glob('exp4_*blocos_LRU.txt'):
        dados_arquivo = extrair_dados_arquivo(arquivo)
        if dados_arquivo:
            dados_lru.append(dados_arquivo)
    
    for arquivo in Path('resultados').glob('exp4_*blocos_ALEATORIO.txt'):
        dados_arquivo = extrair_dados_arquivo(arquivo)
        if dados_arquivo:
            dados_aleatorio.append(dados_arquivo)
    
    if not dados_lru or not dados_aleatorio:
        print("Dados insuficientes para experimento 4")
        return
    
    # Ordenar por tamanho da cache
    dados_lru.sort(key=lambda x: x['tamanho_cache'])
    dados_aleatorio.sort(key=lambda x: x['tamanho_cache'])
    
    tamanhos_lru = [d['tamanho_cache'] for d in dados_lru]
    hit_rates_lru = [d['hit_rate_global'] for d in dados_lru]
    
    tamanhos_aleatorio = [d['tamanho_cache'] for d in dados_aleatorio]
    hit_rates_aleatorio = [d['hit_rate_global'] for d in dados_aleatorio]
    
    plt.figure(figsize=(10, 6))
    plt.plot(tamanhos_lru, hit_rates_lru, 'bo-', linewidth=2, markersize=8, label='LRU')
    plt.plot(tamanhos_aleatorio, hit_rates_aleatorio, 'rs-', linewidth=2, markersize=8, label='Aleatória')
    
    plt.xlabel('Tamanho da Cache (bytes)')
    plt.ylabel('Taxa de Acerto (%)')
    plt.title('Impacto da Política de Substituição na Taxa de Acerto')
    plt.grid(True, alpha=0.3)
    plt.xscale('log', base=2)
    plt.legend()
    
    plt.tight_layout()
    plt.savefig('graficos/impacto_politica_substituicao.png', dpi=300, bbox_inches='tight')
    plt.show()

def gerar_tabela_largura_banda():
    """Tabela: Largura de Banda da Memória"""
    dados_wt = []
    dados_wb = []
    
    # Buscar arquivos do experimento 5
    for arquivo in Path('resultados').glob('exp5_*_WT.txt'):
        dados_arquivo = extrair_dados_arquivo(arquivo)
        if dados_arquivo:
            dados_wt.append(dados_arquivo)
    
    for arquivo in Path('resultados').glob('exp5_*_WB.txt'):
        dados_arquivo = extrair_dados_arquivo(arquivo)
        if dados_arquivo:
            dados_wb.append(dados_arquivo)
    
    if not dados_wt or not dados_wb:
        print("Dados insuficientes para experimento 5")
        return
    
    # Criar DataFrames
    colunas = ['Cache (KB)', 'Bloco (B)', 'Associatividade', 'Leituras MP', 'Escritas MP', 'Total MP']
    
    df_wt = pd.DataFrame(columns=colunas)
    df_wb = pd.DataFrame(columns=colunas)
    
    for dados in dados_wt:
        cache_kb = dados['tamanho_cache'] // 1024
        linha = {
            'Cache (KB)': cache_kb,
            'Bloco (B)': dados['tamanho_linha'],
            'Associatividade': dados['associatividade'],
            'Leituras MP': dados['leituras_mp'],
            'Escritas MP': dados['escritas_mp'],
            'Total MP': dados['leituras_mp'] + dados['escritas_mp']
        }
        df_wt = pd.concat([df_wt, pd.DataFrame([linha])], ignore_index=True)
    
    for dados in dados_wb:
        cache_kb = dados['tamanho_cache'] // 1024
        linha = {
            'Cache (KB)': cache_kb,
            'Bloco (B)': dados['tamanho_linha'],
            'Associatividade': dados['associatividade'],
            'Leituras MP': dados['leituras_mp'],
            'Escritas MP': dados['escritas_mp'],
            'Total MP': dados['leituras_mp'] + dados['escritas_mp']
        }
        df_wb = pd.concat([df_wb, pd.DataFrame([linha])], ignore_index=True)
    
    # Adicionar linha de médias
    media_wt = {
        'Cache (KB)': 'MÉDIA',
        'Bloco (B)': '',
        'Associatividade': '',
        'Leituras MP': int(df_wt['Leituras MP'].mean()),
        'Escritas MP': int(df_wt['Escritas MP'].mean()),
        'Total MP': int(df_wt['Total MP'].mean())
    }
    df_wt = pd.concat([df_wt, pd.DataFrame([media_wt])], ignore_index=True)
    
    media_wb = {
        'Cache (KB)': 'MÉDIA',
        'Bloco (B)': '',
        'Associatividade': '',
        'Leituras MP': int(df_wb['Leituras MP'].mean()[:-1]),  # Excluir linha de média do cálculo
        'Escritas MP': int(df_wb['Escritas MP'].mean()[:-1]),
        'Total MP': int(df_wb['Total MP'].mean()[:-1])
    }
    df_wb = pd.concat([df_wb, pd.DataFrame([media_wb])], ignore_index=True)
    
    # Salvar tabelas
    print("\\n=== TABELA WRITE-THROUGH ===")
    print(df_wt.to_string(index=False))
    
    print("\\n=== TABELA WRITE-BACK ===")
    print(df_wb.to_string(index=False))
    
    # Salvar em arquivo
    with open('resultados/tabela_largura_banda.txt', 'w') as f:
        f.write("LARGURA DE BANDA DA MEMÓRIA\\n\\n")
        f.write("=== WRITE-THROUGH ===\\n")
        f.write(df_wt.to_string(index=False))
        f.write("\\n\\n=== WRITE-BACK ===\\n")
        f.write(df_wb.to_string(index=False))
    
    # Comparação
    total_wt = df_wt['Total MP'].iloc[-1]
    total_wb = df_wb['Total MP'].iloc[-1]
    
    print(f"\\nCOMPARAÇÃO:")
    print(f"Write-through média: {total_wt} acessos à MP")
    print(f"Write-back média: {total_wb} acessos à MP")
    print(f"Write-back reduz tráfego em {((total_wt-total_wb)/total_wt)*100:.1f}%")

def main():
    """Função principal"""
    # Criar diretório para gráficos
    Path('graficos').mkdir(exist_ok=True)
    
    print("Gerando análises dos experimentos...")
    
    if Path('resultados').exists():
        print("\\n1. Impacto do Tamanho da Cache...")
        gerar_grafico_impacto_tamanho_cache()
        
        print("\\n2. Impacto do Tamanho do Bloco...")
        gerar_grafico_impacto_tamanho_bloco()
        
        print("\\n3. Impacto da Associatividade...")
        gerar_grafico_impacto_associatividade()
        
        print("\\n4. Impacto da Política de Substituição...")
        gerar_grafico_impacto_politica_substituicao()
        
        print("\\n5. Largura de Banda da Memória...")
        gerar_tabela_largura_banda()
        
        print("\\nTodas as análises foram concluídas!")
        print("Gráficos salvos na pasta 'graficos/'")
        print("Tabelas salvas na pasta 'resultados/'")
    else:
        print("Pasta 'resultados' não encontrada. Execute primeiro os experimentos.")

if __name__ == "__main__":
    main()
