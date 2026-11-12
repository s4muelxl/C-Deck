# Apresentação do projeto C-DECK

## 1. O que é o projeto?

O C-DECK é um painel de atalhos para Windows. A ideia é reunir, em uma janela simples, alguns comandos que usamos com frequência no computador, como ajustar o volume, silenciar o áudio e abrir programas.

O projeto foi desenvolvido em linguagem C, utilizando a API Win32 do Windows. Ele funciona como um aplicativo desktop, sem depender de uma página aberta no navegador.

## 2. Por que escolhemos essa ideia?

A proposta foi criar uma versão básica de um painel de atalhos, semelhante à ideia de um Stream Deck, mas feita por software. Além de ser útil no dia a dia, o projeto permite aplicar conceitos estudados em programação, como divisão do código em módulos, funções, leitura e gravação de arquivos e uso de bibliotecas do sistema operacional.

O objetivo não foi reproduzir todas as funções de um equipamento comercial. O C-DECK concentra-se em controles simples e que podem ser demonstrados com facilidade.

## 3. O que o programa faz?

Atualmente, o C-DECK oferece estas funções:

- Aumenta o volume principal do Windows em passos de 5%.
- Diminui o volume principal em passos de 5%.
- Liga ou desliga o mudo do dispositivo de reprodução padrão.
- Envia comandos de play/pause, próxima faixa e faixa anterior para o Windows.
- Abre até três aplicativos configurados pelo usuário.
- Permite alterar os nomes e caminhos dos atalhos pela própria interface.
- Salva as configurações em `config/apps.cfg`.

## 4. Tecnologias utilizadas

- **Linguagem C (C11):** implementação da lógica do programa.
- **Win32 API:** criação da janela, botões e campos de configuração.
- **Windows Core Audio:** acesso ao volume principal e ao estado de mudo do dispositivo de áudio padrão.
- **ShellExecute:** abertura dos programas configurados.
- **MinGW-w64 GCC:** compilação do código no ambiente Windows.
- **Inno Setup:** criação do instalador para Windows.
- **GitHub Actions:** automatização da compilação, execução dos testes e geração dos pacotes.

## 5. Como o código está organizado?

| Arquivo | Responsabilidade |
| --- | --- |
| `src/main.c` | Inicializa o programa, prepara o caminho do arquivo de configuração e chama a interface. |
| `src/interface.c` | Cria a janela principal, os botões e a janela de edição dos atalhos. |
| `src/actions.c` | Executa as ações de áudio e abre os aplicativos. |
| `src/config.c` | Lê e grava o arquivo de configuração. |
| `config/apps.cfg` | Guarda os nomes e caminhos dos três atalhos. |
| `tests/` | Contém testes de leitura, gravação e validação da configuração. |

Essa separação evita concentrar todas as operações em um único arquivo. Por exemplo, a interface coleta os dados dos campos, mas quem grava o arquivo é o módulo `config.c`.

## 6. Como funciona a parte principal?

### Controle de volume

O programa acessa o dispositivo de reprodução padrão do Windows usando as interfaces do Windows Core Audio. Primeiro, obtém o nível atual do volume. Depois, soma ou subtrai 0,05, que representa cinco pontos percentuais, e limita o resultado ao intervalo de 0 a 1.

### Botão de mudo

O programa consulta se o dispositivo está silenciado. Em seguida, inverte esse estado: se estava no mudo, ativa o áudio; se estava com áudio, silencia.

### Controles de mídia

Os três botões enviam ao Windows os comandos multimídia padrão do teclado: play/pause, próxima faixa e faixa anterior. O aplicativo de música ou vídeo que estiver preparado para receber esses comandos decide como executá-los.

### Abertura de aplicativos

Os nomes e caminhos ficam no arquivo `config/apps.cfg`. Quando o programa inicia, o módulo de configuração lê esse arquivo. Ao clicar em um atalho, o programa usa `ShellExecute` para solicitar ao Windows a abertura do caminho configurado.

### Editor de atalhos

O botão **Configurar atalhos** abre uma janela com campos para o nome e o caminho de cada um dos três botões. O usuário pode alterar os dados, remover um atalho deixando os dois campos vazios e salvar as mudanças.

Antes de gravar, o programa verifica se cada atalho tem nome e caminho preenchidos em conjunto. O módulo de configuração escreve primeiro em um arquivo temporário e só substitui o arquivo principal quando a gravação termina corretamente.

## 7. Exemplo do arquivo de configuração

```ini
[app1]
name=Bloco de Notas
path=C:\Windows\System32\notepad.exe

[app2]
name=Calculadora
path=C:\Windows\System32\calc.exe

[app3]
name=Paint
path=mspaint.exe
```

O campo `name` define o texto exibido no botão. O campo `path` informa qual executável ou comando deve ser aberto. A edição pode ser feita pelo aplicativo ou diretamente no arquivo.

## 8. Testes e compilação

O repositório possui testes para o leitor e o gravador da configuração. Eles verificam situações como arquivo vazio, seções inválidas, espaços, comentários, campos incompletos e gravação seguida de nova leitura.

O fluxo de integração contínua do GitHub Actions compila o aplicativo, executa os testes e prepara os pacotes para Windows. A execução automatizada confirma a compilação e os testes; ainda é importante realizar testes de uso em um computador Windows para validar o comportamento da interface, do áudio e dos atalhos no ambiente real.

## 9. Como executar uma demonstração

1. Abrir o C-DECK.
2. Clicar para aumentar e diminuir o volume.
3. Ativar e desativar o mudo.
4. Abrir um dos aplicativos pelos botões.
5. Clicar em **Configurar atalhos**.
6. Alterar o nome ou caminho de um atalho, salvar e mostrar que o botão principal foi atualizado.
7. Mostrar o arquivo `config/apps.cfg` para explicar onde os dados ficam armazenados.

## 10. Limitações e possíveis melhorias

Esta versão possui três atalhos configuráveis, controla o áudio do dispositivo de reprodução padrão e envia três comandos multimídia globais. O volume muda em passos fixos de 5%.

Ainda não há macros, perfis, plugins, controle de volume por aplicativo ou integração com um Stream Deck físico. Como próximas melhorias, seria possível adicionar mais botões, perfis de configuração, ações personalizadas e controles de mídia.

## 11. Conclusão

Com o C-DECK, colocamos em prática o desenvolvimento de um aplicativo desktop em C para Windows. O projeto envolve interface gráfica, chamadas a recursos do sistema operacional, leitura e gravação de arquivos, organização modular e automação de compilação.

A versão atual reúne controles básicos de áudio e atalhos em uma única janela e permite administrar os atalhos sem sair do programa. Também deixa uma base organizada para futuras melhorias.

---

**Repositório:** https://github.com/s4muelxl/C-Deck  
**Aplicativo e instalador:** https://github.com/s4muelxl/C-Deck/releases/latest  
**Página do projeto:** https://c-deck.vercel.app/
