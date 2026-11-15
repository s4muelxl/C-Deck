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
| accent | #C7F36B | Seleção e estado ativo |
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
