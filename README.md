# Projeto Multithread - MC504A 1S2025
<figure>
  <img src="decoracao_fila.jpg" alt="Desenho de pessoas na fila de um banheiro">
  <figcaption>Fonte: <a href="https://pt.vecteezy.com/">Vectreezy.com</a></figcaption>
</figure>

## Sobre
Primeiro projeto realizado para a disciplina de Sistemas Operacionais, turma A, Prof. Islene Calciolari Garcia. O projeto busca implementar uma solução, utilizando semáforos, mutex e variáveis de condição para o <b>Problema do Banheiro Unissex</b>, apresentado na Seção 6.2 do livro <a href="https://greenteapress.com/wp/semaphores/">The Little Book of Semaphores</a>, de Allen Downey.

## Integrantes
<ul>
  <li>João Takaki - 260545</li>
  <li>Lucas D'Andréa - 206009</li>
  <li>Lucca Amodio - 239062</li>
  <li>Matheus Ponte - 247277</li>
</ul>

## O problema
Downey introduz o problema contando uma breve história sobre uma amiga que trabalhava em um andar de uma grande empresa. Porém, nesse andar só havia um banheiro masculino, com o feminino mais próximo ficando dois andares acima. A funcionária, então, sugeriu ao seu chefe que convertesse o banheiro em unissex, para que atendesse ambos os gêneros. O chefe concordou, porém adicionou as seguintes regras:
<ul>
  <li>Não pode haver mulheres e homens no banheiro ao mesmo tempo; e</li>
  <li>Não pode haver mais de três empregados no banheiro ao mesmo tempo.</li>
</ul>

## A solução
O problema pode ser tratado, computacionalmente, como um <b>problema de sincronização multithread</b>, em que cada thread representa uma pessoa que precisa utilizar um recurso, no caso, um banheiro, porém com as restrições apresentadas anteriormente.

A solução baseou-se no princípio que, quando a primeira pessoa entra no banheiro, ele torna-se exclusivo para aquele gênero, podendo aceitar apenas pessoas do mesmo. Após um tempo definido (que foi escolhido como 3 segundos), um cronômetro inicia e pessoas do mesmo gênero ainda podem adentrar, caso haja vaga. Ao zerar o cronômetro, a entrada é bloqueada e aguarda-se a saída de todas as pessoas que estão no banheiro. Então, o banheiro torna-se exclusivo para o outro sexo, e o ciclo reinicia-se. Dessa forma, garante-se que todas as threads sejam atendidas, evitando starvation.

Três variáveis controlam o acesso ao banheiro e a impressão dos estados: 
<ul>
  <li><code>mtx</code> (mutex): protege o acesso concorrente a variáveis compartilhadas, como <code>banheiro_ocupado</code>, <code>sexo_atual</code>, o array <code>banheiro</code>, entre outras, garantindo que apenas uma thread por vez possa modificar esses estados compartilhados.</li>
  <li><code>Pessoa.cond</code>: atributo da estrutura Pessoa que, de acordo com o mutex, coloca a thread para dormir (caso esteja esperando a vez de usar o banheiro) ou acordar (caso seja a vez de usar o banheiro). 
  <li><code>sem_estados</code> (semáforo): usado em torno das chamadas da função de impressão de visualização do estado do banheiro (<code>imprime_visualizacao()</code>), o que garante sua atomicidade e protege contra condições de corrida, evitando que a saída não fique embaralhada.</li>
</ul>

Quando uma thread está dormindo, representa uma pessoa na fila esperando para entrar no banheiro. Quando a thread acorda, representa uma pessoa utilizando o banheiro. É importante notar que, quando uma thread termina sua execução, ou seja, uma pessoa deixou o banheiro, a mesma acorda a próxima thread que esteja na fila do mesmo gênero que o seu, indicando que é sua vez.

## Uso de LLMs 
Usamos o Chat GPT como um "norteador" da solução, isto é, como um assistente para saber em que parte da solução deveríamos implementar o semáforo e o mutex, sem que isso seja codificado diretamente em C. Solicitamos um pseudocódigo para nos auxiliar, como pode ser conferido no <a href="https://chatgpt.com/share/68068b4c-6860-800f-ab4a-b935b1e87c76">chat compartilhado</a>.

## Link do vídeo de apresentação
