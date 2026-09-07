# SkateGame — Teste do Vertical Slice 0.2

Este checklist foi criado para o primeiro teste local no Unreal Engine 5.8.

## Objetivo

Validar a base de física antes de importar personagem, skate, animações e cenário final.

O protótipo ainda usa geometria simples. O foco deste teste é comportamento, não qualidade visual.

## Controles

| Ação | Teclado | Gamepad |
| --- | --- | --- |
| Remar | W | Face Button Bottom |
| Virar | A / D | Left Stick X |
| Frear | S | Left Trigger |
| Ollie | Space | Face Button Right |
| Kickflip | Q | Left Shoulder |
| Shove-it | E | Right Shoulder |
| Reset | R | Special Right |
| Câmera | Mouse | Right Stick |

## Overlay de debug

Por padrão o protótipo mostra:

- estado atual: `Grounded`, `Pop`, `Airborne`, `Landing` ou `Bail`;
- velocidade aproximada em km/h;
- número de contatos de roda, de 0 a 4.

O overlay pode ser desativado em `ASkateCharacter::bShowSkateDebug`.

## Teste 1 — repouso

1. Abrir um level vazio.
2. Pressionar Play.
3. Aguardar o skate cair sobre o piso criado pelo GameMode.
4. Confirmar que o estado termina em `Grounded`.
5. Confirmar que o contador normalmente fica em `4/4` sobre superfície plana.
6. Verificar se o deck não vibra de forma excessiva.

Resultado esperado: o skate deve repousar alguns centímetros acima do piso devido aos pontos virtuais de roda/suspensão.

## Teste 2 — push, direção e freio

1. Pressionar W algumas vezes.
2. Fazer curvas suaves com A/D.
3. Segurar S para frear.

Resultado esperado:

- momentum progressivo;
- pouca derrapagem lateral em baixa/média velocidade;
- direção mais perceptível quando o skate já está em movimento;
- frenagem reduzindo velocidade sem parar instantaneamente.

## Teste 3 — banks e superfícies inclinadas

1. Passar pelos banks provisórios.
2. Observar o contador de rodas.

Resultado esperado:

- `4/4` em superfícies estáveis;
- `2/4` ou `3/4` durante transições e aproximações de borda são aceitáveis;
- o skate não deve alternar rapidamente entre `Grounded` e `Airborne` em uma rampa suave.

## Teste 4 — ollie

1. Ganhar alguma velocidade.
2. Pressionar Space.

Sequência esperada no overlay:

`Grounded -> Pop -> Airborne -> Landing -> Grounded`

Falha esperada durante tuning inicial: se a aterrissagem ocorrer com ângulo muito alto ou impacto extremo, o estado pode mudar para `Bail`.

## Teste 5 — kickflip

1. Pressionar Q em solo estável.
2. Observar rotação longitudinal do skate.
3. Repetir em velocidades diferentes.

Resultado esperado: o comando combina pop com impulso angular. A rotação ainda não possui assistência de captura/landing; esta etapa serve para validar o modelo físico.

## Teste 6 — shove-it

1. Pressionar E em solo estável.
2. Observar rotação horizontal do skate.

Resultado esperado: pop + rotação no eixo normal ao solo.

## Teste 7 — bail e reset

1. Tentar aterrissar um kickflip fora de ângulo ou provocar queda.
2. Confirmar estado `Bail` quando detectado.
3. Pressionar R.

Resultado esperado: velocidades linear/angular são zeradas, o skate retorna ao último ponto seguro e o state machine volta a operar normalmente.

## O que anotar durante o primeiro teste

Os seguintes dados são especialmente úteis para tuning:

- FPS aproximado;
- sensação da velocidade;
- quantidade de vibração em repouso;
- frequência com que o contador perde contatos de roda em piso plano;
- altura do ollie;
- se kickflip gira pouco ou demais;
- se shove-it gira pouco ou demais;
- se a direção está lenta ou nervosa;
- se o board quica ao pousar;
- casos em que deveria ocorrer `Bail` e não ocorre;
- casos em que ocorreu `Bail` incorretamente.

## Parâmetros principais para tuning

Em `ASkateboardActor`:

- `PushImpulse`
- `MaxSpeed`
- `RollingResistance`
- `BrakeDrag`
- `GroundGrip`
- `OllieImpulse`
- `SteeringTorque`
- `Wheelbase`
- `TrackWidth`
- `WheelRadius`
- `SuspensionRestLength`
- `SuspensionStrength`
- `SuspensionDamping`
- `SelfRightingStrength`
- `MaxLandingTiltDegrees`
- `MaxLandingVerticalSpeed`
- `FlipAngularImpulse`
- `ShoveAngularImpulse`

## Próxima etapa após o teste

Depois de validar esta base:

1. assistência de captura de flip no momento correto;
2. detecção de nose/tail e board-side para landing;
3. rail/ledge detection para grinds;
4. visual real de deck, trucks e rodas;
5. personagem skeletal mesh + IK dos pés;
6. animações de push, crouch, pop, landing e bail;
7. ragdoll do rider;
8. câmera dinâmica por velocidade e estado.
