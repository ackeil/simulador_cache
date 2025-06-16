Objetivos
Realizar simulações de uma memória cache totalmente configurável (isto é, alguns parâmetros como a política de escrita, tamanho do bloco, número de blocos, número de blocos por conjunto e tempos de leitura e escrita poderão ser configurados) a fim de analisar o efeito dos diversos parâmetros da memória cache no seu desempenho. 

Assim, será necessário desenvolver um programa (em qualquer linguagem de programação) que realize a simulação de uma memória cache associativa por conjunto com arquitetura configurável. O programa deve produzir uma saída com diversas informações sobre a simulação para realizar uma análise posterior. 

Atenção! Cópias de código de qualquer fonte não serão aceitas (o trabalho será zerado). 

Metodologia 
O programa deve receber como entrada um arquivo texto com os endereços (e operação) da simulação (um por linha) e os parâmetros das memórias cache e principal. Os parâmetros de entrada, além do arquivo de endereços, são:

Memória Cache
Política de escrita: 0 - write-through e 1 - write-back;
Tamanho da linha: deve ser potência de 2, em bytes;
Número de linhas: deve ser potência de 2;
Associatividade (número de linhas) por conjunto: deve ser potência de 2 (mínimo 1 e máximo igual ao número de linhas);
Tempo de acesso quando encontra (hit time): em nanossegundos;
Política de Substituição: LRU (Least Recently Used) ou Aleatória;
Memória Principal
Tempos de leitura/escrita: em nanossegundos.
O programa deve produzir um arquivo texto, cujo nome será requisitado na entrada dos parâmetros, com diversas informações sobre a simulação. As informações são:

Todos os parâmetros de entrada: assim é possível verificar os parâmetros utilizados;
Total de endereços no arquivo de entrada: especificar o número de endereços de escrita, leitura e a soma dos dois;
Total de escritas e leituras da memória principal;
Taxa de acerto (hit rate): especificar esta taxa por leitura, escrita e global (colocar ao lado a quantidade);
Tempo médio de acesso da cache (em ns): utilizar a fórmula vista em aula;
Todas as saídas que forem números reais devem ter 4 casas decimais. Ao término da simulação, deve-se atualizar a memória principal com as caches alteradas (caso necessário).

Formato do Arquivo
Dois arquivos serão fornecidos para realizar as simulações. Um arquivo menor para testes chamado 
 (100 entradas) e outro maior para as simulações chamado 
 (51.200 entradas). As entradas destes arquivos são por linha, onde cada linha possui um endereço de 32 bits (em hexadecimal) e uma letra (maiúscula) representando a operação (R – leitura e W – escrita). Um bloco do arquivo é mostrado como exemplo a seguir:

0020a858 R
05fea840 W
001947a0 R
0020a868 R
001947c0 R
Nenhum dos dois arquivos deve ter o formato ou o seu conteúdo modificado.

Especificação das Análises
Para realizar a análise, será necessário executar diversos experimentos utilizando diferentes configurações. Para todos os casos, o tempo de um acerto na cache é 5ns e o tempo de escrita/leitura da memória é 70ns. Todas as análises devem ser escritas com base nos resultados produzidos pelo programa e gráficos produzidos, quando requisitados. 

As seguintes análises deverão ser realizadas:

Impacto do Tamanho da Cache

Realize experimentos com um bloco de 128 bytes, política write-through, LRU, associatividade de 4 blocos, variando o número de blocos da cache. Comece com 8 blocos e aumente, em potências de 2, até que a taxa de acerto fique insensitiva ao tamanho da cache. Faça um gráfico (linha) da taxa de acerto em função do tamanho da cache em bytes. Responda a seguinte questão:

Explique porque o gráfico da taxa de acerto x tamanho da cache possui esta forma.
Impacto do Tamanho do Bloco

Realize experimentos variando o tamanho do bloco de 8 a 4Kb bytes, em potências de 2, para uma memória cache de 8 Kbytes com política write-through, LRU, associatividade de 2 blocos. Faça um gráfico (linha) da taxa de acerto em função do tamanho do bloco. Responda as seguintes questões:

Explique porque o gráfico da taxa de acerto versus tamanho do bloco possui esta forma. Mais especificamente, explique a relevância da localidade espacial para a forma desta curva.
Qual é o tamanho do bloco ótimo?
Impacto da Associatividade

Realize experimentos com um bloco de 128 bytes, política write-back, LRU, variando a associatividade entre 1 e 64, em potências de 2, para uma memória cache de 8 Kbytes. Faça um gráfico (linha) da taxa de acerto em função da associatividade. Responda a seguinte questão:

Explique porque o gráfico da taxa de acerto x associatividade possui esta forma.
Impacto da Política de Substituição

Realize experimentos com um bloco de 128 bytes, política write-through, associatividade de 4 blocos, variando o número de blocos da cache e a política de substituição: LRU e aleatória. Comece com 16 blocos e aumente, em potências de 2, até que a taxa de acerto fique insensitiva ao tamanho da cache. Faça um gráfico (linha) da taxa de acerto em função do tamanho da cache em bytes. Responda a seguinte questão:

Explique porque as curvas das políticas de substituição do gráfico taxa de acerto x tamanho da cache possuem esta forma.
Largura de Banda da Memória

Realize experimentos para medir o tráfego total da memória gerado por uma cache com política write-through versus write-back. Simule memórias caches de 8 Kb e 16 Kb, tamanhos de blocos com 64 e 128 bytes, e associatividades de 2 e 4 blocos e LRU. Faça uma tabela para a política write-through e write-back com os parâmetros, o número de leituras e escritas na memória principal e o total de leituras e escritas. Após a última linha da tabela, coloque a média dos valores das colunas de leitura e escrita na memória principal. Responda a seguinte questões:

Qual cache possui o menor tráfego de memória: write-trhough ou write-back? Por quê?
Avaliação Global

Dados todos os experimentos realizados. Responda as seguintes questões:

Qual é o melhor projeto de cache dentre todas as configurações simuladas? Explique como você chegou a esta decisão.
É possível simular as demais formas de mapeamento com o seu simulador? Caso afirmativo, explique como. Caso negativo, explique o porquê. 
Qual foi a parte mais difícil do processo de desenvolver o seu simulador de cache?
