# C-DECK

**Painel de atalhos para Windows, desenvolvido em C11 e Win32 API.**

O C-DECK reúne controles básicos de áudio e atalhos para aplicativos em uma pequena janela desktop. A página em `web/` é apenas a apresentação do projeto; as ações do computador são executadas pelo aplicativo Windows.

## Recursos

- Aumentar e diminuir o volume principal em passos de 5%.
- Alternar o mute do dispositivo de reprodução padrão.
- Abrir até três aplicativos definidos pelo usuário.
- Carregar a configuração a partir da pasta do executável.
- Interface Win32 nativa, sem runtime web.

## Baixar e executar

1. Abra a aba **Actions** do repositório.
2. Entre na execução mais recente do workflow **C-Deck CI** que terminou com sucesso.
3. Na seção **Artifacts**, baixe `C-Deck-Windows`.
4. Extraia o arquivo `C-Deck-Portable.zip` para uma pasta.
5. Execute `C-Deck.exe`.

O pacote portátil contém:
- `C-Deck.exe`
- `config/apps.cfg`
- `README.md`

O executável precisa permanecer ao lado da pasta `config`. Não mova apenas o EXE para outro local.

## Configurar os atalhos

Edite `config/apps.cfg` em um editor de texto. O arquivo usa três blocos:

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

- `name`: texto mostrado no botão.
- `path`: caminho absoluto do executável ou comando disponível no PATH.

Salve o arquivo e reinicie o C-DECK para carregar os novos nomes e caminhos.

## Compilar no Windows

Requisitos: Windows 10/11 e MinGW-w64 GCC no PATH.

Execute na raiz do repositório:

```powershell
.\scripts\build.ps1
```

O executável será criado em `build/C-Deck.exe`.

## Estrutura

- `src/main.c`: inicialização e localização da configuração.
- `src/interface.c`: janela e botões Win32.
- `src/actions.c`: ações de áudio e abertura de aplicativos.
- `src/config.c`: leitura de configuração.
- `config/apps.cfg`: atalhos de exemplo.
- `tests/`: teste do parser.
- `web/`: landing page estática.
- `.github/workflows/ci.yml`: compilação, teste e empacotamento automatizados.

## Limitações atuais

- O áudio é controlado no dispositivo de reprodução padrão do Windows.
- O volume muda em passos fixos de 5%.
- São suportados três atalhos configuráveis.
- A configuração é carregada ao iniciar o programa.
- Ainda não há macros, perfis, plugins, controle individual por aplicativo ou integração com hardware Stream Deck.

## Licença

MIT. Consulte [LICENSE](LICENSE).
