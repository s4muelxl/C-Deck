# Auditoria e atualização da landing page — 17/11/2026

## Baseline verificado

- Repositório: \`s4muelxl/C-Deck\`.
- Branch de origem: \`main\`, commit \`0d4025e788616cee8998435ad1a4d847b35bc1f0\`.
- Produção Vercel: deployment \`dpl_Es1cSU8oCotQSGhHKwoaXVFY3zX2\`, estado \`READY\`, alias \`https://c-deck.vercel.app/\`.
- O deployment de produção apontava para o mesmo commit de \`main\` auditado.
- A landing page publicada no commit-base ainda se identificava como v1.3.1, tinha navegação limitada a marca/GitHub, um conteúdo concentrado em uma coluna, dependência de Google Fonts e CSS com passagens de estilo acumuladas.
- O simulador web era uma grade 3×3 reduzida, com volume, mute, mídia e três exemplos de atalhos. O próprio texto já informava que os controles eram locais e não executavam ações reais.
- A versão pública do instalador foi confirmada em \`installer/C-Deck.iss\` como 1.3.1 e na release \`latest\`; por isso, esta atualização mantém a comunicação comercial limitada a 1.3.1 e não anuncia recursos ainda não distribuídos na release pública.

## Melhorias aplicadas

- Hero em duas colunas no desktop, com hierarquia de mensagem e CTA mais clara.
- Navegação por âncoras no desktop e menu móvel nativo com \`details/summary\`, sem dependência JavaScript adicional.
- Sistema visual consolidado em tokens, superfícies grafite, tipografia de sistema e verde-lima reservado a indicadores/estados.
- CSS reestruturado em uma folha única, com breakpoints para tablet, mobile e telas estreitas.
- Simulador preservado com os mesmos IDs e contratos de interação; a grade 3×3 continua explicitamente identificada como demonstração reduzida.
- Seções de recursos, fluxo de uso, tecnologia, FAQ e CTA final com conteúdo limitado ao que a versão pública 1.3.1 documenta.
- Metadados canonical, Open Graph, Twitter Card e JSON-LD \`SoftwareApplication\`.
- \`robots.txt\` e \`sitemap.xml\` para descoberta e indexação.
- Remoção da dependência de fontes externas para reduzir conexões de terceiros e evitar bloqueio de renderização por Google Fonts.
- Estados de foco visíveis, suporte a \`prefers-reduced-motion\`, \`prefers-contrast\`, \`forced-colors\` e alvos de toque de pelo menos 44 px nos controles do simulador.

## Escopo e arquitetura

As alterações ficam restritas a \`web/\` e documentação do site. O desktop continua C11/C++/Win32; nenhum runtime web foi adicionado à aplicação nativa.

## Validação

- O workflow \`C-Deck Web UX\` executa \`node --check web/main.js\` e \`node --test web/tests/ux-contract.test.js\`.
- A produção-base foi confirmada como \`READY\` pela API de deployments da Vercel.
- A inspeção HTTP/renderizada direta da URL pública não pôde ser executada neste ambiente de trabalho por indisponibilidade de resolução DNS/rede. O status \`READY\` confirma o estado da plataforma, mas não substitui a checagem HTTP externa e a inspeção visual final após o deploy.

## Iteração de interação móvel

- O menu móvel usa o elemento nativo `details/summary`, sem biblioteca adicional.
- Ao escolher um destino ou pressionar Escape, o menu fecha; Escape devolve o foco ao acionador.
- O contrato é coberto por teste unitário do script no ambiente Node.

## Evidência visual automatizada

- Foi adicionado `web/visual-check.cjs`, executado em Chromium headless apenas no CI.
- A suíte verifica ausência de overflow horizontal em 10 larguras (320–1440 px), navegação móvel por seleção/Escape, canonical/JSON-LD, sitemap, robots, assets locais e erros JavaScript.
- O workflow `C-Deck Web Visual QA` gera capturas full-page desktop (1440 px), mobile (390 px) e mobile estreito (320 px), anexadas como artefato temporário de 14 dias.
- Playwright/Chromium é dependência exclusiva do runner; não é enviado ao site nem ao aplicativo desktop.

## Verificação de produção após integração

- O workflow `C-Deck Production Web Smoke` é acionado por push em `main` quando arquivos web mudam e também pode ser executado manualmente.
- Ele aguarda a nova página responder HTTP 200 e exige o título e canonical desta versão, evitando aceitar por engano o HTML antigo ainda servido durante a propagação.
- Também exige HTTP 200 e conteúdo esperado em `robots.txt` e `sitemap.xml`.
- Se a Vercel não publicar ou a URL continuar servindo a versão anterior, o smoke test falha e a publicação não é declarada concluída.


## Atualização de baseline e conteúdo — 17/11/2026

- A branch de publicação foi recriada a partir do HEAD atual de `main`: `ec1d529d4b641ee871ea9273205d9061aea7705a`, que integra a release 1.4.0 e a auditoria GDI final.
- A versão pública do produto é `1.4.0`, confirmada por `VERSION`, `README.md`, `installer/C-Deck.iss` e `src/interface.c`. A landing page, o JSON-LD e os testes de versão agora usam o mesmo valor.
- O conteúdo comercial foi atualizado para as capacidades verificadas da 1.4.0: grade 5×3 com 15 slots, quatro perfis independentes, biblioteca de ações e Multi Action com até oito macros e 12 etapas por macro.
- A demonstração web continua reduzida a 3×3 e local/ilustrativa. O texto informa que ela não controla o Windows, não executa ações reais e representa apenas uma amostra da grade desktop 5×3.
- A atualização preserva a hierarquia visual, navegação móvel nativa, estados de foco, suporte a contraste/movimento reduzido, ausência de fontes externas e capturas Playwright em 10 larguras (320–1440 px).
- A PR #18 anterior partia de `0d4025e` e estava 18 commits atrás da main. O compare identificou sobreposição em `.github/workflows/ci.yml`, `web/index.html` e `web/tests/ux-contract.test.js`. A nova branch foi criada sobre a main atual; os três caminhos foram reconciliados sem descartar a verificação de versão da CI nem os testes de release.
- Nenhum arquivo `src/`, `installer/`, `VERSION` ou lógica desktop foi modificado por esta atualização web. C11/C++/Win32 permanecem intactos.
- A validação do HEAD final será considerada concluída somente após os workflows Web UX, Web Visual QA e C-Deck CI passarem e a prévia Vercel correspondente ao SHA ficar `READY`.
- O deployment de produção ainda aponta para o commit anterior. A publicação permanece condicionada à revisão formal, integração aprovada, Vercel Production `READY` e smoke test HTTP 200 com título/canonical, `robots.txt` e `sitemap.xml`.
