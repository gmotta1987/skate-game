# SkateGame

Protótipo de jogo 3D de skate com foco em sensação realista, animação fluida, física responsiva e qualidade visual de alto nível.

## Stack

- Unreal Engine 5.8
- C++ + Blueprints
- Enhanced Input (previsto para evolução do input layer)
- Chaos Physics
- Control Rig / IK
- Motion Warping
- Niagara
- MetaSounds
- Lumen + Nanite

## Estado atual — Vertical Slice 0.1

A primeira base jogável já está implementada inteiramente em C++, sem depender de assets externos:

- skate físico com Chaos;
- remada / impulso;
- direção;
- resistência de rolamento;
- aderência lateral;
- freio;
- ollie;
- câmera third-person com lag;
- reset do rider;
- suporte a teclado, mouse e gamepad;
- arena de teste criada em runtime;
- mesh temporária para o shape usando o cubo padrão do Unreal.

O `ASkateboardActor` é o corpo físico principal. O `ASkateCharacter` funciona como rider/câmera e acompanha o skate. Essa separação permite adicionar animação, IK, ragdoll e tricks sem criar dois sistemas de física concorrentes.

## Controles atuais

| Ação | Teclado / mouse | Gamepad |
|---|---|---|
| Remar | `W` | Face Button Bottom |
| Virar | `A / D` | Left Stick X |
| Frear | `S` | Left Trigger |
| Ollie | `Space` | Face Button Right |
| Câmera | Mouse | Right Stick |
| Reset | `R` | Menu / Special Right |

## Como testar agora

1. Instale Unreal Engine 5.8.
2. Clone o repositório.
3. Gere os project files a partir de `SkateGame.uproject`, se necessário.
4. Compile o target `SkateGameEditor`.
5. Abra o projeto no Unreal Editor.
6. Crie ou abra um nível vazio.
7. Pressione **Play**.

O `SkateGameMode` cria automaticamente uma arena provisória de aproximadamente 50 x 50 metros com chão, ledge e banks simples. Isso permite testar a física antes da entrada dos assets finais.

## Objetivo do MVP

Construir primeiro um **vertical slice jogável**, antes de expandir mapa, customização e conteúdo.

### Fase 1 — Skate básico
- [x] Controle inicial de direção
- [x] Push / remada
- [x] Freio
- [x] Ollie
- [x] Física inicial do skate
- [x] Câmera third-person responsiva
- [ ] Ajuste fino de trucks, rodas e curvas
- [ ] Detecção de aterrissagem limpa / bail

### Fase 2 — Tricks
- Kickflip / heelflip
- Shove-it
- Grinds e slides
- Manuals
- Sistema de aterrissagem e quedas

### Fase 3 — Animação
- Full-body IK
- Ajuste dos pés no shape
- Blend de animações por velocidade e inclinação
- Reação corporal à física do skate
- Ragdoll em quedas

### Fase 4 — Visual
- Cenário urbano realista
- Nanite
- Lumen
- Materiais PBR
- Iluminação cinematográfica
- Motion blur e câmera polida

### Fase 5 — Game loop
- Linhas e desafios
- Pontuação por tricks
- Combos
- Sessões livres
- Replay / câmera lenta

## Estrutura

```text
Config/                      Configurações do Unreal
Content/                     Assets e Blueprints criados no Unreal Editor
Source/SkateGame/Actors/     Skate e objetos físicos
Source/SkateGame/Characters/ Rider e câmera
Source/SkateGame/Game/       GameMode e bootstrap do protótipo
Source/SkateGame/            Módulo C++ principal
docs/                        Arquitetura, gameplay e planejamento
SkateGame.uproject           Projeto Unreal
```

## Próxima implementação

O próximo marco técnico é substituir o shape provisório por uma composição física de **deck + trucks + rodas**, melhorar contato com rampas/transições e adicionar o primeiro sistema de trick state (`Grounded`, `Pop`, `Airborne`, `Landing`, `Bail`). Depois disso entram personagem humano, animações e IK.

## Princípio do projeto

O objetivo é atingir a mesma categoria de qualidade visual e sensação de movimento da referência, mas usando código, assets, animações, cenários e identidade próprios.
