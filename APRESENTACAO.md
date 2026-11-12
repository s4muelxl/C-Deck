# Apresentação do projeto C-DECK

## 1. O que é o projeto?

O C-DECK é um painel de atalhos para Windows. A ideia é reunir, em uma janela simples, alguns comandos que usamos com frequência no computador, como ajustar o volume, silenciar o áudio e abrir programas.

O projeto foi desenvolvido em linguagem C, utilizando a API Win32 do Windows. Ele funciona como um aplicativo desktop, sem depender de uma página aberta no navegador.

## 2. Por que escolhemos essa ideia?

A proposta foi criar uma versão básica de um painel de atalhos, semelhante à ideia de um Stream Deck, mas feita por software. Além de ser útil no dia a dia, o projeto permite aplicar conceitos estudados em programação, como divisão do código em módulos, funções, leitura de arquivos e uso de bibliotecas do sistema operacional.

O objetivo não foi reproduzir todas as funções de um equipamento comercial. A primeira versão concentra-se em controles simples e que podem ser demonstrados com facilidade.

## 3. O que o programa faz?

Atualmente, o C-DECK oferece estas funções:

- Aumenta o volume principal do Windows em passos de 5%.
- Diminui o volume principal em passos de 5%.
- Liga ou desliga o mudo do dispositivo de reprodução padrão.
- Abre até três aplicativos configurados pelo usuário.
- Lê os nomes e caminhos dos aplicativos em um arquivo externo, chamado `config/apps.cfg`.

Os atalhos de exemplo são Bloco de Notas, Calculadora e Paint. Eles podem ser substituídos editando o arquivo de configuração.

## 4. Tecnologias utilizadas

- **Linguagem C (C11):** implementação da lógica do programa.
- **Win32 API:** criação da janela e dos controles da interface no Windows.
- **Windows Core Audio:** acesso ao volume principal e ao estado de mudo do dispositivo de áudio padrão.
- **ShellExecute:** abertura dos programas configurados.
- **MinGW-w64 GCC:** compilação do código no ambiente Windows.
- **Inno Setup:** criação do instalador para Windows.
- **GitHub Actions:** automatização da compilação, execução do teste do parser e geração dos pacotes.

## 5. Como o código está organizado?

O código foi separado por responsabilidade para facilitar a leitura e a manutenção:

| Arquivo | Responsabilidade |
| --- | --- |
| `src/main.c` | Inicializa o programa, prepara o caminho do arquivo de configuração e chama a interface. |
| `src/interface.c` | Cria a janela e os botões do aplicativo. |
| `src/actions.c` | Executa as ações de áudio e abre os aplicativos. |
| `src/config.c` | Lê e interpreta o arquivo de configuração. |
| `config/apps.cfg` | Guarda os nomes e caminhos dos três atalhos. |
| `tests/` | Contém o teste do leitor de configuração. |

Essa separação evita concentrar todas as operações em um único arquivo. Por exemplo, a interface solicita uma ação, mas quem executa a alteração de áudio é o módulo `actions.c`.

## 6. Como funciona a parte principal?

### Controle de volume

O programa acessa o dispositivo de reprodução padrão do Windows usando as interfaces do Windows Core Audio. Primeiro, obtém o nível atual do volume. Depois, soma ou subtrai 0,05, que representa cinco pontos percentuais, e limita o resultado ao intervalo de 0 a 1.

### Botão de mudo

O programa consulta se o dispositivo está silenciado. Em seguida, inverte esse estado: se estava no mudo, ativa o áudio; se estava com áudio, silencia.

### Abertura de aplicativos

Os nomes e caminhos ficam no arquivo `config/apps.cfg`. Quando o programa inicia, o módulo de configuração lê esse arquivo. Ao clicar em um atalho, o programa usa `ShellExecute` para solicitar ao Windows a abertura do caminho configurado.

## 7. Exemplo do arquivo de configuração

O arquivo usa seções para identificar cada atalho:

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

O campo `name` define o texto exibido no botão. O campo `path` informa qual executável ou comando deve ser aberto. Assim, não é necessário alterar o código-fonte para trocar os programas.

## 8. Testes e compilação

O repositório possui um teste para o leitor do arquivo de configuração. O fluxo de integração contínua do GitHub Actions compila o aplicativo, executa esse teste e prepara os pacotes para Windows.

A compilação automatizada e a geração do instalador foram concluídas com sucesso no fluxo publicado no GitHub. Isso confirma que o processo de build e empacotamento funcionou. Ainda é importante realizar testes de execução em um computador Windows para validar o comportamento da interface, do áudio e dos atalhos no ambiente real.

## 9. Como executar uma demonstração

Durante a apresentação, uma sequência simples permite mostrar as principais partes do projeto:

1. Abrir o C-DECK.
2. Clicar para aumentar o volume e observar a alteração.
3. Clicar para diminuir o volume.
4. Ativar e desativar o mudo.
5. Abrir um dos aplicativos pelos botões.
6. Mostrar o arquivo `config/apps.cfg` e explicar como trocar um atalho.

Se houver tempo, também é possível mostrar a organização das pastas no GitHub e o workflow de compilação.

## 10. Limitações e possíveis melhorias

Esta é uma primeira versão funcional, com escopo reduzido. No momento:

- O controle de áudio atua sobre o dispositivo de reprodução padrão.
- O volume é alterado em passos fixos de 5%.
- Existem três atalhos configuráveis.
- As configurações são carregadas quando o programa inicia.
- Ainda não há macros, perfis, plugins, controle de volume por aplicativo ou integração com um Stream Deck físico.

Como evolução, seria possível permitir mais botões, criar perfis de atalhos, adicionar ações personalizadas e melhorar a edição das configurações pela própria interface.

## 11. Conclusão

Com o C-DECK, colocamos em prática o desenvolvimento de um aplicativo desktop em C para Windows. O projeto envolve interface gráfica, chamadas a recursos do sistema operacional, leitura de arquivo externo, organização modular e automação de compilação.

A versão atual atende ao objetivo inicial de reunir controles básicos de áudio e atalhos em uma única janela. Também deixa uma base organizada para futuras melhorias.

---

**Repositório:** https://github.com/s4muelxl/C-Deck  
**Aplicativo e instalador:** https://github.com/s4muelxl/C-Deck/releases/latest  
**Página do projeto:** https://c-deck.vercel.app/
