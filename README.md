# SkateGame

Protótipo de jogo 3D de skate com foco em sensação realista, animação fluida, física responsiva e qualidade visual de alto nível.

## Stack

- Unreal Engine 5.8
- C++ + Blueprints
- Enhanced Input
- Chaos Physics
- Control Rig / IK
- Motion Warping
- Niagara
- MetaSounds
- Lumen + Nanite

## Objetivo do MVP

Construir primeiro um **vertical slice jogável**, antes de expandir mapa, customização e conteúdo.

### Fase 1 — Skate básico
- Controle de aceleração e direção
- Push com o pé
- Freio
- Ollie
- Física inicial do skate
- Câmera third-person responsiva

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
Config/             Configurações do Unreal
Content/            Assets e Blueprints (gerados no Unreal Editor)
Source/SkateGame/   Código C++ principal
docs/               Arquitetura, gameplay e planejamento
SkateGame.uproject  Projeto Unreal
```

## Primeiros passos

1. Instale Unreal Engine 5.8.
2. Clone o repositório.
3. Clique com o botão direito em `SkateGame.uproject` e gere os project files, se necessário.
4. Compile o target `SkateGameEditor`.
5. Abra o projeto no Unreal Editor.

## Princípio do projeto

O objetivo é atingir a mesma categoria de qualidade visual e sensação de movimento da referência, mas usando código, assets, animações, cenários e identidade próprios.
