# Durma Tranquilamente

Foi desenvolvido pela dupla um jogo baseado formato _Escape Room_, no qual o jogador permanece em uma casa e deve interagir com os móveis e objetos do ambiente, descobrindo uma sequência oculta de ações que deve realizar antes de deitar para dormir. O objetivo do jogo é dormir em segurança, sem que uma morte ocorra. 

![Início do Jogo](screenshots/1.png)

## Como Executar

## Como Jogar

## Aplicação dos Conhecimentos de Computação Gráfica

O código segue a estrutura dos laboratórios práticos realizados ao longo da disciplina. A seguir, será detalhado o processo de desenvolvimento e o uso de conceitos de Computação Gráfica estudados na disciplina, assim como os requisitos do jogo que foram implementados.

### Malhas Poligonais Complexas e Mapeamento de Texturas

O jogo possui um total de 15 objetos de complexidade variadas. Os modelos 3D dos objetos foram obtidos do site [Rig Models](https://rigmodels.com/index.php). Cada modelo possui um arquivo .obj, um arquivo .mtl e uma ou mais imagens de textura. 

Para obter as informações dos arquivos foi utilizada a mesma função de leitura dos laboratórios 4 e 5, porém para enviar as informações para os _shaders_ foi necessário adaptar a função de desenho, pois era precis informar para cada objeto, qual era sua imagem de textura. Também foi utilizada uma terceira função que relacionada cada objeto com sua imagem de textura, para facilitar a passagem dessa informação para os _shaders_ na função de desenho. 

![Início do Jogo](screenshots/5.png)

Todos objetos do jogo têm sua cor definida a partir de uma textura e já vieram com as coordenadas de textura definidas no arquivo .obj. Alguns objetos, além da cor oriunda da textura, também possuem cálculo de iluminação. Os modelos de iluminação utilizados estão descritos a seguir. 

Para o carregamento dos objetos e das texturas foram utilizados conhecimentos adiquiridos durante a realização dos laboratórios 4 e 5. Esses conhecimentos também foram extendidos, uma vez que os objetos do jogo, diferente dos objetos dos laboratórios, possuiam informações em arquivo .mtl.

### Transformações Geométricas Controladas pelo Usuário

### Câmera Livre e Câmera Look At

O jogo desenvolvido possui os dois tipos de cãmera: câmera livre e cãmera look at. Ao iniciar o jogo, o jogador está dentro da casa e tem uma visão obtida através da câmera livre, podendo se movimentar pelo cômodo utilizando o teclado e o mouse. 

![Início do Jogo](screenshots/8.png)

_adicionar parágrafo sobre implementação da câmera livre_

O jogador também tem a possibilidade de apertar a tecla _c_ e mudar o tipo de câmera virtual de câmera livre para câmera look at. Quando o jogador altera o tipo de câmera ele vai para o exterior da casa.

![Início do Jogo](screenshots/7.png)

Quando a tecla é pressionada e a flag _useLookAt_ é ativada a posição da câmera é calculada em coordenadas esféricas. As variáveis *g_CameraPhi*, *g_CameraTheta*, *g_CameraDistance* definem sua posição. a câmera está fixamente olhando para a origem, onde está centrada a casa do jogo. Quando o botão esquerdo do mouse é pressionado, o movimento do cursor atualiza os ângulos *g_CameraTheta* e *g_CameraPhi*, fazendo a câmera orbitar ao redor da casa. Também é possível se aproximar ou se afastar da casa usando _scroll_ do mouse.

A câmera look at foi implementada da mesma maneira vista no laboratório 2, com uma única diferença. O jogo não poderia permitir que o jogador orbitasse livremente pela casa e visualizasse seu interior por baixo. Para evitar isso, o ângulos *g_CameraPhi* foi limitado na função _CursorPosCallback()_. Se ele for menor que 0.01, ele é fixado em 0.01. Assim a sensação é que a câmera virtual "bate no chão".

### Mais de uma Instância para Algum Objeto

O objeto BREAD que está localizado em cima da mesa tem duas intâncias. As informações dos arquivos .obj e .mtl são obtidas apenas uma vez. Foram definidas duas matrizes de transformação geométrica, cada uma transladada de uma maneira. A rotação dos objetos também é uma diferença entre as matrizes.

![Início do Jogo](screenshots/3.png)

### Tipos de Testes de Intersecção

### Modelos de Iluminação e Modelos de Interpolação

### Movimentação com Curva de Bézier Cúbica

## Animações Baseadas em Tempo

## Contribuições

### Aline

### Mariana

## Uso de Códigos Gerados por Inteligência Artificial