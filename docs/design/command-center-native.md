# C-Deck 2.0 — Native Command Center

> **Status:** proposta de design para a interface desktop nativa. Esta é uma especificação de direção e implementação; não afirma que os módulos e estados descritos já estejam disponíveis na versão atual.

## Princípios

1. **Native first:** a aplicação desktop permanece em C/C++ com Win32 e APIs do Windows. Não introduzir Electron, Tauri, WebView, Chromium ou runtime web.
2. **Identidade própria:** linguagem industrial e minimalista; preto, grafite, branco e neutros como base. Verde-limão é uma cor de assinatura usada com parcimônia para seleção e estados ativos, não como decoração generalizada.
3. **Foco no deck:** a grade 5×3 é o centro visual e funcional da janela. Sidebar e inspector apoiam a edição sem competir com ela.
4. **Progressive disclosure:** configurações avançadas aparecem no inspector ou no Action Studio; a tela Workspace permanece rápida de ler.
5. **Honestidade funcional:** contadores, estados de dispositivo, métricas e ações só aparecem quando há fonte real no núcleo. Conceitos não implementados devem ser identificados como proposta.

## Estrutura da janela

A composição de referência usa quatro regiões, com dimensões em unidades lógicas (DIP), não pixels físicos:

| Região | Largura/altura de referência | Responsabilidade |
|---|---:|---|
| Top bar | 60–68 DIP de altura | Navegação de módulos, perfil ativo e comandos globais |
| Sidebar | 200–224 DIP | Perfis, páginas/coleções e categorias da biblioteca |
| Workspace | flexível | Grade 5×3, edição direta e contexto do deck |
| Inspector | 280–320 DIP | Propriedades da tecla/ação selecionada |
| Status bar | 32–36 DIP de altura | Estado real do aplicativo, perfil e feedback operacional |

A grade deve manter cinco colunas e três linhas em modo normal. As teclas usam proporção quadrada, espaçamento uniforme e tamanho calculado pelo espaço disponível; não devem ser dimensionadas por coordenadas absolutas fixas.

### Breakpoints funcionais

- **Janela ampla (>= 1360 DIP):** sidebar e inspector visíveis; workspace central com respiro.
- **Janela média (1120–1359 DIP):** inspector recolhível; sidebar permanece completa.
- **Janela estreita (900–1119 DIP):** sidebar reduzida a rail de ícones com nomes acessíveis; inspector em painel temporário.
- **Modo compacto:** esconder navegação secundária e inspector; manter grade, perfil e saída rápida do modo compacto. Nunca reduzir o alvo interativo abaixo do mínimo definido para teclado/mouse.
- **Abaixo da largura mínima suportada:** impedir layout quebrado por meio de WM_GETMINMAXINFO ou oferecer modo compacto; não sobrepor controles.

Os valores acima são pontos iniciais de validação, não limites já implementados. Medir em 100%, 125%, 150% e 200% de DPI, incluindo monitores com DPI diferente.

## Design tokens propostos

| Token | Valor | Uso |
|---|---|---|
| canvas | #0B0D10 | Fundo principal |
| surface-1 | #12161B | Sidebar e superfícies base |
| surface-2 | #191F26 | Cartões e teclas |
| surface-3 | #222A33 | Hover e painéis elevados |
| line-subtle | #2B333D | Separadores discretos |
| line-strong | #46515E | Bordas e limites importantes |
| text-primary | #F3F5F7 | Texto principal |
| text-secondary | #A5AFBA | Descrições |
| text-disabled | #69737F | Elementos indisponíveis |
| accent | #9DFF00 | Verde oficial da marca; seleção e estado ativo |
| danger | #F06B6B | Erro/ação destrutiva |
| warning | #E6B85C | Aviso que exige atenção |

O acento verde-limão deve ocupar áreas pequenas (indicador, contorno ou ícone). Não usar grandes superfícies saturadas. Foco de teclado deve continuar perceptível em escala de cinza e em alto contraste; não depender apenas da cor para indicar seleção.

## Componentes e estados

### Tecla da grade

- Ícone vetorial ou pictograma consistente, nome curto e categoria opcional.
- Normal: superfície grafite, borda sutil e texto de alto contraste.
- Hover: elevação/borda discreta, sem mudança brusca de cor.
- Pressed: compressão visual breve; executar somente após validação do comando.
- Selected: contorno de acento + indicador secundário; não confundir com ação em execução.
- Disabled: contraste reduzido, mas rótulo legível e motivo disponível em tooltip/inspector.
- Empty: affordance de adicionar ação sem parecer botão destrutivo.

### Sidebar e navegação

- Categorias com ícone monocromático e rótulo; contagem só quando calculada da biblioteca real.
- Perfil ativo identificado por marcador e texto, não apenas pela cor.
- Navegação superior: Workspace, Action Studio, Macro Flow, Perfis e Dispositivos. Itens sem implementação devem permanecer fora do menu de produção ou marcados como indisponíveis em protótipo.

### Inspector

- Título contextual (tecla, ação ou perfil), campos agrupados e hierarquia clara.
- Ações primárias no rodapé do painel; cancelar e salvar devem ser semanticamente distintos.
- Campos inválidos devem apresentar mensagem próxima ao campo e não depender apenas de toast.
- Alterações não persistidas devem ser identificáveis; cancelar não pode gravar estado parcial.

### Feedback e métricas

- Toast para confirmação transitória; status bar para estado persistente.
- Não exibir CPU, RAM, número de execuções ou estado de dispositivo até existir medição/fonte real.
- Erros de API/Windows devem preservar o último estado confirmado e explicar que a consulta falhou, sem inventar um estado.

## Arquitetura de apresentação (C/C++)

A camada visual deve consumir estado e emitir intenções; ela não deve executar ações do Windows diretamente.

- **View state:** estruturas C/C++ contendo seleção, perfil visível, modo compacto e estado de edição.
- **Layout:** cálculo de retângulos em DIP, conversão via DPI da janela e tratamento de resize.
- **Controls/rendering:** controles Win32 reutilizáveis; custom draw limitado a componentes que precisem de identidade própria.
- **Commands:** a UI emite IDs/intenções para a camada de aplicação; o Action Engine valida e executa.
- **Persistence:** salvar por APIs existentes e operações transacionais já testadas; a camada visual não manipula arquivos de configuração diretamente.

Manter a renderização GDI atual como baseline segura. Avaliar Direct2D/DirectWrite em uma prova isolada somente se houver benefício mensurável (tipografia, escalabilidade ou animação) e custo/complexidade aceitáveis. Qualquer adoção precisa ser incremental, compilável no toolchain atual e coberta por testes; não iniciar reescrita ampla da interface.

## Acessibilidade e qualidade

- Ordem de tabulação previsível; setas navegam entre as 15 teclas; Enter/Space seleciona ou aciona conforme contexto.
- Nomes acessíveis para ícones, teclas e controles sem texto visível.
- Foco visível, contraste suficiente, escala de texto e suporte a alto contraste.
- Respeitar redução de movimento; animações curtas e não essenciais.
- Testar redimensionamento, maximização, restauração, múltiplos monitores e DPI por janela.
- Validar estados de erro, loading, vazio, seleção, pressed e disabled.

## Plano de implementação seguro

1. Congelar o contrato visual e mapear os controles/estados já existentes na interface Win32.
2. Introduzir tokens e helpers de layout em módulo separado, sem alterar semântica das ações.
3. Aplicar tokens primeiro à top bar e sidebar; validar tabulação e DPI.
4. Aplicar à grade 5×3 e ao inspector, preservando hit-testing e IDs existentes.
5. Revisar menus, seletor de ícones, perfis e modo compacto.
6. Adicionar testes para cálculo de layout/escala quando extraível do Win32 e executar build no runner Windows.
7. Fazer smoke test manual com capturas em 100/125/150/200% antes de remover o draft.

## Dependência entre PRs

A PR #16 (codex/c-deck-1.4-engineering) modifica src/interface.c extensivamente e introduziu o editor nativo de macros. A PR #17 (codex/c-deck-1.4-design) permanece isolada em web/**, workflow web e este documento de especificação. **Não editar src/interface.c nesta frente enquanto a engenharia estiver em revisão ativa.** Depois de a engenharia estabilizar, rebase/coordenação deve ocorrer em um conjunto explícito de mudanças, comparando novamente os SHAs e os arquivos tocados.

A landing page continua descrevendo apenas capacidades já existentes na versão publicada. Este documento é uma direção futura e não promove os recursos da PR #16 como funcionalidades já lançadas.


## Auditoria do baseline nativo (branch de engenharia, HEAD 45285b88)

A leitura de src/interface.c na branch codex/c-deck-1.4-engineering encontrou o seguinte estado. Estes pontos são observações de código, não validação visual manual:

| Elemento | Implementação observada | Implicação para o redesign |
|---|---|---|
| Top bar | 52 px | Aumentar a hierarquia/altura para a faixa proposta somente após revisar posições dos controles |
| Sidebar | 200 px fixa | Converter para largura adaptável e rail em janela estreita |
| Inspector | 320 px fixos | Tornar recolhível e adaptar campos à largura disponível |
| Status bar | 28 px | Reavaliar altura e separar estado de execução de métricas |
| Grade | 5×3; gap 14 DIP normal / 8 DIP mini; teclas entre 44 e 108 DIP | Calcular pelo espaço disponível, manter alvo mínimo acessível e limitar o crescimento em monitores grandes |
| Janela normal mínima | 980×640 | Conferir se os breakpoints propostos podem ser suportados sem reduzir os alvos ou sobrepor controles |
| Mini-Deck mínimo | 440×320 | Preservar acesso de retorno ao modo normal e validar legibilidade |
| DPI | WM_SIZE e WM_GETMINMAXINFO presentes; não foi encontrado handler WM_DPICHANGED na revisão deste HEAD | Implementar tratamento per-monitor DPI antes de declarar responsividade completa |

A paleta nativa atual usa fundo #15171B e acento oficial #9DFF00 (RGB 157,255,0), com fundo de seleção #18281A. A direção nova mantém o verde oficial, mas restringe seu uso a seleção/estado ativo e evita glow amplo. A proposta de canvas mais escuro é uma mudança visual futura; deve ser validada por contraste em monitores reais.

O layout ainda usa valores fixos para top bar, sidebar, inspector e posições de alguns botões. O cálculo da grade já é parcialmente responsivo, mas o layout geral não pode ser considerado per-monitor DPI até existir WM_DPICHANGED e teste em múltiplos monitores. Não modificar esses pontos nesta PR enquanto a PR #16 estiver em revisão ativa.


## Layout contract em C11 (15/11/2026)

A primeira parte executável do contrato de layout foi extraída para `src/ui_layout.c` / `src/ui_layout.h`, sem dependência Win32. O módulo calcula em DIP as regiões de top bar, sidebar, workspace, inspector, status bar e os 15 retângulos quadrados da grade.

- Breakpoints por largura da área cliente: wide a partir de 1360 DIP; medium a partir de 1120 DIP; narrow a partir de 900 DIP. Em narrow a sidebar vira rail de 64 DIP; em medium o inspector fica recolhido; em wide ambos aparecem.
- Compact é explícito e mantém top bar de 48 DIP, status bar de 30 DIP e gap de 8 DIP; o alvo mínimo de 440×320 DIP é coberto por teste.
- O cálculo centraliza a grade 5×3 e preserva teclas quadradas entre 44 e 108 DIP. O limite superior evita que a grade fique desproporcional em janelas/monitores grandes; o limite inferior preserva o alvo interativo. Se a área não comportar 44 DIP, retorna `DECK_UI_LAYOUT_TOO_SMALL` e zera todo o resultado (modo unsupported, regiões e slots vazios), evitando que um consumidor use retângulos parciais de uma tentativa inválida.
- `deck_ui_dip_to_px` converte DIP para pixels físicos e `deck_ui_px_to_dip` faz a conversão inversa, ambos com arredondamento ao inteiro mais próximo, fallback de 96 DPI e saturação em int32. O caminho pixels→DIP permite que a camada Win32 converta `GetClientRect` antes de chamar o cálculo de layout.
- Testes cobrem wide/medium/narrow/compact, os 15 slots, limites e não sobreposição, dimensões insuficientes, argumentos inválidos e escalas 96/120/144/192 DPI.

**Limite de integração:** o módulo ainda não está ligado ao executável nem substitui o layout de `src/interface.c`. Não há handler `WM_DPICHANGED` adicionado nesta rodada. O próximo passo, após estabilização da PR #16, é consumir estes retângulos no ciclo Win32, tratar `WM_DPICHANGED` e validar em sessão gráfica real; até lá, os testes comprovam apenas o contrato matemático independente da janela.


## Contrato de navegação por teclado da grade (15/11/2026)

O módulo independente `src/ui_navigation.c` / `src/ui_navigation.h` define o movimento de foco entre as 15 teclas sem depender de Win32:

- Setas esquerda/direita movem uma coluna; cima/baixo movem uma linha.
- As bordas são limitadas (clamp), sem wrap-around. Por exemplo, pressionar esquerda na primeira coluna mantém o foco nela.
- O índice de foco é separado do estado de seleção e da execução: mover o foco nunca seleciona uma ação nem a executa.
- Índices fora de 0–14, direções desconhecidas e ponteiro de saída nulo são rejeitados; em caso de erro, a saída permanece inalterada.
- O contrato é coberto por teste para todos os 15 slots e quatro direções, incluindo bordas e argumentos inválidos.

**Limite de integração:** este módulo ainda não recebe mensagens `WM_KEYDOWN` nem está ligado ao executável. A integração deve ocorrer em coordenação com a PR #16, que modifica `src/interface.c`; a camada Win32 deverá mapear as setas para este contrato e manter Enter/Space separados do movimento de foco. A compilação/teste C11 não comprova a ordem de foco nem o comportamento visual de foco em uma sessão gráfica.


## Verificação automatizada de contraste dos tokens (15/11/2026)

O teste nativo de tema calcula a luminância relativa sRGB e a razão de contraste dos tokens, usando a fórmula WCAG:

- Texto primário e secundário: mínimo de 4,5:1 em canvas, superfícies 1–3 e fundo de seleção.
- Texto desabilitado: mínimo interno de 3:1 nos mesmos fundos; o estado continua identificado semanticamente como indisponível.
- Indicadores de acento, erro, aviso e foco: mínimo de 3:1 nos mesmos fundos.

A verificação roda no contrato C11 em Linux e MinGW/Windows. O vínculo com `libm` existe apenas no executável de teste para a função `pow`; não adiciona dependência ao runtime desktop.

Esta é uma verificação matemática dos tokens de cor, não uma medição de pixels renderizados. Contraste final de componentes, fontes, antialiasing, alto contraste do Windows e foco em controles nativos ainda exigem inspeção visual em sessão real.


## Contrato de hit-testing da grade (15/11/2026)

O módulo nativo de layout também oferece `deck_ui_grid_hit_test`, que converte uma posição de ponteiro em DIP no índice de uma tecla da grade:

- Usa limites semiabertos: esquerda/topo inclusivos e direita/base exclusivos. Isso evita que um clique na borda compartilhada seja atribuído a duas teclas.
- Os espaços entre teclas e áreas fora da grade retornam `DECK_UI_HIT_TEST_OUTSIDE`; não há seleção implícita de uma tecla vizinha.
- Antes de resolver o índice, valida as 15 regiões: dimensões quadradas consistentes, limites dentro do workspace, coordenadas representáveis e ausência de sobreposição.
- Em argumentos/layout inválidos ou posição fora da grade, o índice de saída permanece inalterado.
- A função só identifica o slot. Não altera foco/seleção e não executa ações.

A camada Win32 deverá converter as coordenadas físicas recebidas em mensagens de mouse para DIP com `deck_ui_px_to_dip` antes de chamar o contrato. O resultado deverá ser encaminhado à camada de estado/command, mantendo a execução fora do hit-testing.

Os testes cobrem o centro de todos os 15 slots, limites semiabertos, gaps, área externa, layout corrompido e preservação do valor de saída. O contrato continua independente do runtime Win32 e não constitui validação visual interativa.


## Alvos de toque do simulador web (15/11/2026)

Os controles Reiniciar e Som do cabeçalho do simulador ilustrativo usam alvos mínimos de 44×44 CSS px, inclusive em larguras de viewport até 360 px. A faixa do cabeçalho foi elevada para 52 px para acomodar os controles sem sobreposição. O contrato é específico da demonstração web; não representa nem substitui a validação dos controles Win32 do desktop.


## Conversão de retângulos sem seams em DPI fracionário (15/11/2026)

O contrato de layout agora inclui `deck_ui_rect_dip_to_px` e `deck_ui_rect_px_to_dip`. Ao contrário de converter posição e tamanho separadamente, os helpers convertem as quatro bordas do retângulo (esquerda, topo, direita e base) e só então recompõem largura/altura.

Isso evita gaps ou sobreposição de um pixel entre regiões adjacentes em escalas fracionárias como 125% e 150%, quando arredondar a largura independentemente da posição poderia produzir resultados diferentes. Origens negativas são suportadas para coordenadas de janelas em monitores posicionados à esquerda/acima da origem virtual.

- DPI zero mantém o fallback documentado de 96 DPI.
- Dimensões negativas são argumentos inválidos; coordenadas de borda ou dimensões resultantes não representáveis retornam `DECK_UI_RECT_SCALE_OUT_OF_RANGE`.
- O destino permanece inalterado em qualquer erro; entrada e saída podem apontar para a mesma estrutura.
- Os testes cobrem 96/120/144/192 DPI, alinhamento de bordas compartilhadas, origem negativa, retângulos de área zero, argumentos inválidos e overflow.

O helper é um contrato matemático C11. Ele ainda não trata `WM_DPICHANGED` nem altera o tamanho/posição da janela; o runtime Win32 deverá usar o retângulo sugerido pelo Windows na mensagem e aplicar estes helpers ao converter a geometria de layout para desenho e hit-testing.


## Navegação por teclado do simulador web (15/11/2026)

A demonstração web 3×3 usa roving tabindex para evitar nove paradas consecutivas na ordem de tabulação:

- Tab entra na grade por uma única tecla; o foco atual passa a ser a única tecla com `tabindex="0"`.
- As setas movem o foco horizontal ou verticalmente dentro das três linhas/colunas.
- Home/End levam ao primeiro/último slot da linha atual; Ctrl+Home/Ctrl+End levam ao primeiro/último slot da grade. Meta+Home/End oferece o mesmo atalho em navegadores/plataformas que usam Command.
- As bordas usam clamp (sem wrap); o navegador não rola a página quando uma tecla é tratada pela grade.
- Mover o foco não ativa comandos demonstrativos. Enter/Space continuam com a semântica nativa do botão e executam apenas a simulação local.
- As instruções são fornecidas por texto visualmente oculto e associado ao grupo via `aria-describedby`.

O contrato nativo C11 espelha o mesmo modelo com `deck_ui_grid_focus_boundary`: sem modificador, calcula o início/fim da linha; com `entire_grid=true`, calcula o primeiro/último slot global. O helper só calcula o destino de foco e nunca seleciona ou executa uma ação. A integração desses comandos às mensagens de teclado Win32 ainda depende de coordenação com a PR #16.


## Acento de assinatura reservado ao estado selecionado (16/11/2026)

A landing page mantém o acento neutro (`#EEF1F5`) para foco genérico, links e controles. O verde-lima oficial (`#9DFF00`) fica reservado ao estado ativo/selecionado da tecla de reprodução no simulador, alinhando a demonstração ao token `DECK_UI_STATE_SELECTED` da paleta C11 sem colorir indiscriminadamente a interface. O indicador usa uma borda e um brilho discretos; o ícone também assume o acento enquanto a reprodução simulada está ativa. Em `forced-colors`, o estado usa a cor de sistema `Highlight`, preservando a semântica em alto contraste.

## Feedback persistente do simulador

A barra de status do simulador web distingue mensagens transitórias de estado demonstrativo estável. Após o feedback temporário, o texto deve refletir a combinação atual de reprodução simulada e mute simulado; não deve retornar a um estado genérico de pronto enquanto a reprodução estiver marcada como ativa. Os rótulos sempre incluem o contexto de simulação e não representam estado do player ou do Windows.

## Validação integrada de geometria e hit-testing por DPI

A suíte de layout combina agora os contratos de geometria e entrada em um percurso de regressão: calcula a grade 5×3 em DIP, converte cada retângulo de tecla para pixels físicos, simula um clique no centro físico de cada tecla, converte a coordenada de volta para DIP e confirma que o hit-test resolve o mesmo slot.

O percurso é executado para os 15 slots em quatro composições (wide, medium, narrow e compact mínimo) e quatro escalas (96, 120, 144 e 192 DPI), totalizando 240 verificações. Isso detecta regressões em que o arredondamento de bordas, a escala do retângulo ou a conversão das coordenadas mudem a identidade da tecla.

O teste valida os contratos matemáticos independentes. Ele não simula `WM_DPICHANGED`, não substitui o tratamento de coordenadas da janela Win32 e não equivale a um teste interativo com mouse em monitores físicos.

## Conversão explícita da paleta para GDI / COLORREF (16/11/2026)

A paleta semântica continua armazenando cores como `0xRRGGBB`, independente de Win32. Antes de passar um token para APIs GDI que recebem `COLORREF` (por exemplo, criação de brushes/pens), a camada de apresentação deve chamar `deck_ui_rgb_to_colorref`. O helper converte a ordem dos canais para `0x00BBGGRR`, não inclui `windows.h` e mantém os contratos de tema compiláveis em C11 portátil. O teste cobre vermelho, verde, azul e o acento oficial `#9DFF00`, prevenindo troca acidental dos canais vermelho/azul durante a futura integração Win32. O helper não cria nem libera objetos GDI.


## Estado de apresentação da grade (16/11/2026)

O módulo independente `src/ui_state.c` / `src/ui_state.h` mantém o estado transitório e de contexto das 15 teclas sem acoplar apresentação, execução ou Win32:

- **Foco** e **seleção** são campos independentes. Mover foco com setas/Home/End não muda a seleção nem executa uma ação.
- **Hover** e **pressed** são estados de ponteiro transitórios. Um pressionamento só vira seleção quando a liberação ocorre sobre a mesma tecla; liberar em outra tecla ou fora da grade cancela o pressionamento sem alterar a seleção anterior.
- Os flags de cada slot podem ser combinados. Assim, uma tecla pode estar selecionada e focada ao mesmo tempo; a camada de renderização decide como combinar contorno de seleção e anel de foco.
- Mudanças de foco por teclado cancelam um pressed antigo para evitar estado visual preso. Limpar estados transitórios preserva foco e seleção.
- Índices inválidos e ponteiros nulos são rejeitados sem modificar o estado. `DECK_UI_SLOT_NONE` representa ausência de hover/seleção/pressed.
- A seleção representa contexto de edição/Inspector, não uma solicitação de execução. O módulo não chama o Action Engine, não persiste dados, não aloca memória e não depende de APIs Win32.

A disponibilidade/desabilitação de uma ação continua vindo do modelo funcional do slot; este contrato não inventa nem infere se uma ação pode ser executada.

**Limite de integração:** o contrato C11 é testado em Linux e Windows/MinGW, mas ainda não está ligado aos eventos `WM_MOUSEMOVE`, `WM_LBUTTONDOWN`, `WM_LBUTTONUP` ou `WM_KEYDOWN` do executável. Essa integração permanece coordenada com a PR #16, que altera `src/interface.c`; nenhum arquivo compartilhado foi modificado nesta rodada.


## Simulador web — ciclo de vida dos efeitos sonoros (16/11/2026)

Os efeitos sonoros do simulador são opcionais e locais ao navegador; não representam áudio do Windows nem fazem parte do runtime desktop.

- O contexto Web Audio é criado sob demanda, somente quando uma interação solicita um efeito.
- Ao ocultar a página, o contexto é suspenso para evitar processamento de áudio em segundo plano.
- Ao sair da página (`pagehide`), o contexto é fechado e a referência é liberada. Se a página retornar pelo cache de navegação, um novo contexto será criado apenas na próxima interação.
- Falhas síncronas ou rejeições de `resume`, `suspend` e `close` são tratadas sem interromper os controles demonstrativos.
- A preferência SOM: ON/OFF continua independente do estado do contexto e é preservada pelo botão de reinicialização da demonstração.

O teste de contrato usa um contexto Web Audio simulado para provocar rejeições assíncronas e verificar suspensão, fechamento e recriação. Isso valida o ciclo de vida do código, não a qualidade acústica ou a disponibilidade de dispositivos em um navegador físico.


## Foco do slider de volume em alto contraste (16/11/2026)

O slider de volume da demonstração web mantém um contorno de foco de teclado ao redor de toda a área interativa, em vez de depender apenas do brilho do thumb. Isso torna o foco mais fácil de localizar quando o usuário navega com Tab/Shift+Tab. Em `forced-colors`, a borda usa a cor de sistema `Highlight`; a indicação não depende de sombras, que podem ser removidas pelo modo de alto contraste do navegador/Windows.

O contrato automatizado verifica a regra de foco normal e a substituição para `Highlight`. Trata-se de uma garantia CSS do simulador web, não de validação do controle nativo Win32.


## Composição de foco com estado semântico (16/11/2026)

O contrato de tema agora oferece `deck_ui_style_for_interaction`, que combina o estado visual primário (`normal`, `hover`, `pressed`, `selected`, `disabled`, `error` ou `warning`) com um indicador de foco independente. Quando `focused` é verdadeiro, a função adiciona `DECK_UI_STYLE_FOCUS_RING` sem substituir `fill`, `border` ou `indicator` do estado primário. Assim, uma tecla selecionada conserva a borda/indicador verde-lima e pode receber simultaneamente um anel de foco visível.

O renderer Win32 deve desenhar o anel separadamente, usando `DECK_UI_COLOR_FOCUS` e respeitando o inset/outset definido pelo componente. Não deve reutilizar o token de foco como borda principal, pois isso apagaria a semântica de seleção. O chamador deve informar `focused` a partir do estado real de foco; esta função não infere foco nem altera estado de navegação.

Os testes verificam a combinação selected+focused, hover+focused, normal sem foco, fallback de estado inválido e paleta nula. O helper continua sendo um contrato C11 puro: ainda não está ligado ao desenho Win32 nem substitui a inspeção visual de foco na janela real.


## Resolver de apresentação por slot (16/11/2026)

O módulo independente `src/ui_presentation.c` / `src/ui_presentation.h` conecta o estado transitório da grade ao contrato de tema, sem acoplar a camada de desenho ao modelo de ações:

- O estado semântico fornecido pelo modelo (`disabled`, `error` ou `warning`) prevalece sobre hover/pressed/selected. O chamador continua responsável por fornecer o estado real da ação; o resolver não inventa disponibilidade.
- Quando o modelo informa `normal`, a precedência de feedback é pressed → selected → hover → normal. Assim, o hover não remove o acento de uma tecla selecionada e o pressed continua oferecendo resposta imediata durante o clique.
- O foco é retornado separadamente para que `deck_ui_style_for_interaction` adicione o anel sem substituir a borda semântica. O foco é suprimido para controles disabled.
- Estados de modelo inválidos, slots fora de 0–14 e argumentos nulos são rejeitados sem alterar a saída.
- O módulo não desenha, não executa ações, não persiste configuração e não depende de Win32. A futura camada Win32 deverá resolver o estado a partir do modelo real do slot e entregar o resultado ao renderer.

A suíte `tests/ui_presentation_test.c` cobre estado normal/hover, selected+hover com foco, pressed+selected, override de disabled/error/warning e preservação da saída em entradas inválidas. Os workflows de UI nativa compilam e executam o novo contrato em Linux e MinGW/Windows, tanto em push quanto em pull request.

**Limite de integração:** este resolver ainda não está ligado ao ciclo de pintura de `src/interface.c`. A integração continua condicionada à coordenação com a PR #16, que altera esse arquivo. Os testes validam a resolução semântica em C11, não a aparência em uma sessão gráfica Windows.


## Validação integral do shell antes da renderização (16/11/2026)

O contrato C11 de layout agora expõe `deck_ui_layout_validate`. A validação não se limita às 15 teclas: ela confere também a geometria das regiões da janela antes de um frame de renderização ser aceito.

- Top bar e status bar precisam ocupar a largura cliente completa e manter as alturas definidas para o modo.
- Sidebar, workspace e inspector precisam formar uma partição contínua do corpo da janela, sem sobreposição, lacuna ou retângulo residual quando um painel está recolhido.
- A visibilidade e as larguras dos painéis devem corresponder ao modo wide, medium, narrow ou compact; larguras nas bordas dos breakpoints não podem ser rotuladas como outro modo.
- A grade precisa manter 15 slots quadrados, gap uniforme, ordem row-major e centralização no workspace.
- Geometria de regiões inválida retorna `DECK_UI_LAYOUT_INVALID_REGIONS`; grade inválida retorna `DECK_UI_LAYOUT_INVALID_GRID`. O validador é somente leitura.

`deck_ui_grid_frame_build` passou a usar esse validador público. Portanto, um shell inconsistente é rejeitado antes de produzir o snapshot para o renderer, mesmo que os 15 retângulos individuais ainda pareçam válidos. Os testes incluem as larguras 900/1119/1120/1359/1360 DIP e mutações isoladas de top bar, sidebar, workspace, inspector, status bar, flags de visibilidade, ordem/gap da grade e preservação da saída do frame.

Isso fortalece o contrato de geometria; não significa que o layout já esteja ligado à janela Win32 nem substitui testes de resize e DPI em uma sessão gráfica real.
