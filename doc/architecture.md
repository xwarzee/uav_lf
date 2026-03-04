# Architecture du système DroneSwarm

> Langue de modélisation : **Lingua-Franca** (cible C++)
> Fichier principal : `src/DroneSwarm.lf`

---

## Table des matières

1. [Vue d'ensemble](#1-vue-densemble)
2. [Structure du projet](#2-structure-du-projet)
3. [Réacteurs](#3-réacteurs)
   - 3.1 [DroneSwarm — main reactor](#31-droneswarm--main-reactor)
   - 3.2 [PositionAggregator](#32-positionaggregator)
   - 3.3 [DroneNode — bank](#33-dronenode--bank)
   - 3.4 [FormationController](#34-formationcontroller)
   - 3.5 [ObstacleDetector](#35-obstacledetector)
   - 3.6 [MissionExecutor](#36-missionexecutor)
   - 3.7 [DroneComms](#37-dronecomms)
4. [Flux de données et connexions](#4-flux-de-données-et-connexions)
5. [Temporisation](#5-temporisation)
6. [Paramétrage runtime](#6-paramétrage-runtime)
7. [Patterns de communication](#7-patterns-de-communication)
8. [Schémas](#8-schémas)

---

## 1. Vue d'ensemble

Le système **DroneSwarm** est une application réactive temps-réel modélisant un essaim de drones UAV. Il est développé en **Lingua-Franca** avec la cible **C++** et repose sur le framework réactif `reactor-cpp`.

Chaque drone est une instance d'un **bank** de réacteurs `DroneNode`. Quatre sous-systèmes — formation, détection d'obstacles, exécution de mission et communications — sont instanciés dans chaque drone. Le nombre de drones est **paramétrable à l'exécution** via l'option `--num_drones`.

```
./bin/DroneSwarm --num_drones 6
```

Le schéma de vue d'ensemble est disponible dans `diagrams/01_system_overview.puml`.

---

## 2. Structure du projet

```
src/
├── DroneSwarm.lf              # main reactor + DroneNode + PositionAggregator
├── MissionCommandInterface.lf # interface stdin → commandes de mission/formation
├── FormationController.lf     # calcul de formation (diamond, line, V, circle)
├── ObstacleDetector.lf        # fusion capteurs, évitement de collision
├── MissionExecutor.lf         # gestion du cycle de mission et des waypoints
└── DroneComms.lf              # réseau maillé, heartbeat, chiffrement

doc/
├── architecture.md        # ce document
└── diagrams/
    ├── 01_system_overview.puml      # vue d'ensemble du système
    ├── 02_drone_node_internals.puml # structure interne d'un DroneNode
    ├── 03_reactors_interfaces.puml  # interfaces (ports, paramètres, timers)
    ├── 04_sequence_mission.puml     # scénario mission + obstacle
    └── 05_timing.puml               # diagramme de temporisation
```

---

## 3. Réacteurs

### 3.1 DroneSwarm — main reactor

**Fichier :** `src/DroneSwarm.lf`

Réacteur principal. Orchestre l'ensemble de l'essaim.

| Élément | Valeur |
|---------|--------|
| Paramètre | `num_drones : int = 4` |
| Lancement | `./bin/DroneSwarm --num_drones N` |

**Instances créées :**

| Instance | Type | Rôle |
|----------|------|------|
| `drones[N]` | `DroneNode` (bank) | N drones de l'essaim |
| `aggregator` | `PositionAggregator` | agrège les N positions |
| `swarm_formation` | `FormationController` (drone_id=0) | contrôleur formation leader |
| `mission_coordinator` | `MissionExecutor` (drone_id=0) | coordinateur de mission |
| `cmd_interface` | `MissionCommandInterface` | interface de commande stdin |

**Connexions principales :**

```
cmd_interface.mission_command       → mission_coordinator.mission_command
cmd_interface.formation_change      → swarm_formation.formation_change
swarm_formation.position_command[N] → drones.formation_command   (N→N)
mission_coordinator.mission_status  → drones.mission_update      (1→N broadcast)
drones.position                     → aggregator.positions        (N→N)
aggregator.aggregated_positions     → drones.neighbor_positions   (1→N, after 10 ms)
```

---

### 3.2 MissionCommandInterface

**Fichier :** `src/MissionCommandInterface.lf`

Pont entre l'opérateur humain et le système réactif. Utilise une **physical action** LF pour injecter des commandes stdin dans la timeline réactive de façon thread-safe : un thread détaché lit `std::cin` et appelle `stdin_command.schedule(line)`.

| Port de sortie | Type | Destination |
|----------------|------|-------------|
| `mission_command` | `string` | `mission_coordinator.mission_command` |
| `formation_change` | `int` | `swarm_formation.formation_change` |

**Commandes reconnues :**

| Catégorie | Commandes |
|-----------|-----------|
| Type de mission | `PATROL` `ESCORT` `RECONNAISSANCE` `SEARCH_RESCUE` `CARGO_DELIVERY` `SURVEILLANCE` `INTERCEPT` |
| Contrôle | `START` `STOP` `ABORT` `PAUSE` `RESUME` `RTB` |
| Formation | `FORMATION 0` (diamond) `FORMATION 1` (line) `FORMATION 2` (V) `FORMATION 3` (circle) |
| Aide | `HELP` |

**Session exemple :**
```
=== DroneSwarm — Interface de commande ===
> PATROL
[MISSION] → PATROL
> START
[MISSION] → START
> FORMATION 3
[FORMATION] → circle
> PAUSE
[MISSION] → PAUSE
> RTB
[MISSION] → RTB
```

---

### 3.3 PositionAggregator

**Fichier :** `src/DroneSwarm.lf`

Collecte les positions de tous les drones via un **multiport** de largeur `num_drones` et publie la carte agrégée toutes les 50 ms.

| Port | Type | Direction |
|------|------|-----------|
| `positions[num_drones]` | `vector<double>` | entrée (multiport) |
| `aggregated_positions` | `map<int, vector<double>>` | sortie |

| Timer | Période |
|-------|---------|
| `aggregate_timer` | 50 ms |

---

### 3.3 DroneNode — bank

**Fichier :** `src/DroneSwarm.lf`

Représente un drone individuel. Instancié N fois via le mécanisme de **bank** Lingua-Franca. Le `bank_index` (0-indexé) est automatiquement assigné par le runtime ; le `drone_id` interne est `bank_index + 1` (1-indexé).

| Paramètre | Valeur par défaut | Description |
|-----------|-------------------|-------------|
| `bank_index` | auto (0..N-1) | indice dans le bank, assigné automatiquement |
| `num_drones` | 4 | taille totale de l'essaim |
| `initial_x/y/z` | 0.0 / 0.0 / 10.0 | position initiale (m) |

| Port d'entrée | Type | Description |
|---------------|------|-------------|
| `formation_command` | `vector<double>` | position cible depuis le leader |
| `mission_update` | `string` | commande de mission (START/STOP) |
| `neighbor_positions` | `map<int, vector<double>>` | positions de tous les voisins |

| Port de sortie | Type | Description |
|----------------|------|-------------|
| `position` | `vector<double>` | position courante {x, y, z} |
| `status` | `string` | statut de mission |
| `threat_detected` | `bool` | alerte obstacle |

**Sous-systèmes instanciés :**

```
formation  = FormationController(drone_id = bank_index+1, swarm_size = num_drones)
obstacle   = ObstacleDetector(drone_id = bank_index+1)
mission    = MissionExecutor(drone_id = bank_index+1)
droneComms = DroneComms(drone_id = bank_index+1)
```

**Logique d'évitement de collision (réaction `neighbor_positions`) :**

Pour chaque voisin à distance `d < 5 m` et `d > 0.1 m`, une force de répulsion inversement proportionnelle à `d²` est appliquée à la vélocité du drone.

| Timer | Période | Rôle |
|-------|---------|------|
| `update_timer` | 100 ms | publication de la position |

---

### 3.4 FormationController

**Fichier :** `src/FormationController.lf`

Calcule la position cible de chaque drone dans la formation. Instancié dans chaque `DroneNode` (pour son propre calcul) et une fois dans `DroneSwarm` comme contrôleur leader (drone_id=0).

| Paramètre | Valeur par défaut | Description |
|-----------|-------------------|-------------|
| `drone_id` | 0 | identifiant du drone (0 = leader) |
| `formation_type` | 0 | type de formation initial |
| `swarm_size` | 4 | nombre total de drones suiveurs |

**Types de formation (`formation_type`) :**

| Valeur | Nom | Algorithme |
|--------|-----|------------|
| 0 | Diamond | 4 points cardinaux ; cercle général pour N > 4 |
| 1 | Line | positionnement linéaire derrière le leader |
| 2 | V-Formation | alternance gauche/droite |
| 3 | Circle | cercle équidistant à rayon `formation_spacing = 10 m` |

**Sortie :** `output[swarm_size] position_command : vector<double>` — une commande par drone suiveur.

**Mise à jour dynamique :** le port `swarm_size_update : int` permet de modifier la taille de l'essaim à l'exécution.

| Timer | Offset | Période |
|-------|--------|---------|
| `formation_timer` | 100 ms | 50 ms |

---

### 3.5 ObstacleDetector

**Fichier :** `src/ObstacleDetector.lf`

Traite les données capteurs (LIDAR / caméras) en coordonnées polaires `[distance, angle, élévation]`, construit une carte d'obstacles et calcule les vecteurs d'évitement.

| Paramètre | Valeur par défaut | Description |
|-----------|-------------------|-------------|
| `drone_id` | 0 | identifiant du drone |
| `detection_range` | 50.0 m | portée de détection |
| `threat_threshold` | 15.0 m | seuil de menace |

**Niveaux de menace :**

| Niveau | Valeur | Distance |
|--------|--------|----------|
| Safe | 0 | > 15 m |
| Caution | 1 | ≤ 15 m |
| Danger | 2 | ≤ 10 m |
| Critical | 3 | ≤ 5 m |

**Évitement inter-drones :** force répulsive si distance < 8 m (seuil distinct de l'évitement d'obstacles statiques).

| Timer | Période | Fréquence |
|-------|---------|-----------|
| `obstacle_timer` | 25 ms | 40 Hz |

---

### 3.6 MissionExecutor

**Fichier :** `src/MissionExecutor.lf`

Gère le cycle de vie des missions : chargement des waypoints, progression, gestion des timeouts, retour à la base (RTB) automatique en cas de niveau critique batterie/carburant/santé.

**Missions supportées :**

| Mission | Timeout | Priorité | Tolérance waypoint |
|---------|---------|----------|-------------------|
| PATROL | 30 min | 1 | 2 m |
| RECONNAISSANCE | 15 min | 2 | 2 m |
| SEARCH_RESCUE | 60 min | 3 | 1 m |
| INTERCEPT | 10 min | 3 | 5 m |
| RTB | — | 2 | 2 m |

**Déclenchement automatique du RTB :** batterie < 20 %, carburant < 15 %, santé système < 50 %.

| Timer | Période | Rôle |
|-------|---------|------|
| `mission_update_timer` | 100 ms | progression waypoints |
| `health_check_timer` | 1 s | surveillance des niveaux critiques |

---

### 3.7 DroneComms

**Fichier :** `src/DroneComms.lf`

Gère le réseau maillé (mesh) inter-drones : découverte des pairs, heartbeat, routage des messages, supervision de la santé du réseau.

| Paramètre | Valeur par défaut | Description |
|-----------|-------------------|-------------|
| `drone_id` | 0 | identifiant du drone |
| `communication_range` | 100.0 m | portée radio |
| `encryption_enabled` | true | chiffrement XOR |

**Types de messages :** POSITION_UPDATE, HEARTBEAT, EMERGENCY (3 retransmissions), routage mesh.

| Timer | Période | Rôle |
|-------|---------|------|
| `broadcast_timer` | 500 ms | diffusion de position |
| `heartbeat_timer` | 1 s | surveillance voisins |
| `mesh_update_timer` | 2 s | mise à jour topologie |
| `discovery_timer` | 5 s | découverte de nouveaux drones |
| `cleanup_timer` | 10 s | purge des voisins inactifs (> 30 s) |

---

## 4. Flux de données et connexions

```
┌──────────────────────────────────────────────────────────────┐
│                    DroneSwarm (N drones)                     │
│                                                              │
│  swarm_formation ──[position_command × N]──► drones[N]      │
│                                                              │
│  mission_coordinator ──[mission_status]──►  drones[N]       │
│         (broadcast 1 → N)                                   │
│                                                              │
│  drones[N] ──[position × N]──► aggregator                   │
│         (multiport N → N)                                    │
│                                                              │
│  aggregator ──[aggregated_positions]──► drones[N]           │
│         (broadcast 1 → N, after 10 ms)                      │
└──────────────────────────────────────────────────────────────┘

Interne à chaque DroneNode :
  obstacle.threat_alert ──► threat_detected (sortie)
  mission.mission_status ──► status (sortie)
  update_timer ──► position (sortie, 100 ms)
```

---

## 5. Temporisation

| Réacteur | Timer | Offset | Période | Fréquence |
|----------|-------|--------|---------|-----------|
| ObstacleDetector | `obstacle_timer` | 0 | 25 ms | 40 Hz |
| PositionAggregator | `aggregate_timer` | 0 | 50 ms | 20 Hz |
| FormationController | `formation_timer` | 100 ms | 50 ms | 20 Hz |
| DroneNode | `update_timer` | 0 | 100 ms | 10 Hz |
| MissionExecutor | `mission_update_timer` | 100 ms | 100 ms | 10 Hz |
| MissionExecutor | `health_check_timer` | 0 | 1 s | 1 Hz |
| DroneComms | `broadcast_timer` | 0 | 500 ms | 2 Hz |
| DroneComms | `heartbeat_timer` | 0 | 1 s | 1 Hz |
| DroneComms | `mesh_update_timer` | 0 | 2 s | 0.5 Hz |
| DroneComms | `discovery_timer` | 0 | 5 s | 0.2 Hz |
| DroneComms | `cleanup_timer` | 0 | 10 s | 0.1 Hz |

Le diagramme de temporisation est disponible dans `diagrams/05_timing.puml`.

---

## 6. Paramétrage runtime

Le paramètre `num_drones` du main reactor est exposé automatiquement comme argument CLI par le runtime `reactor-cpp` :

```bash
# Voir toutes les options disponibles
./bin/DroneSwarm --help

# Lancer avec 6 drones (formation en cercle recommandée)
./bin/DroneSwarm --num_drones 6

# Lancer avec timeout de 60 secondes
./bin/DroneSwarm --num_drones 4 --timeout 60 s

# Mode accéléré (temps logique > temps physique)
./bin/DroneSwarm --num_drones 4 --fast
```

**Impact du changement de `num_drones` :**

| Composant | Adaptation automatique |
|-----------|----------------------|
| `DroneNode[N]` | N instances créées via bank |
| `PositionAggregator` | multiport redimensionné à N |
| `FormationController` (leader) | `swarm_size = N` → sortie multiport de largeur N |
| `FormationController` (par drone) | `swarm_size = N` → formations adaptées |

---

## 7. Patterns de communication

### Multiport N → N (symétrique)
Utilisé pour les connexions dont les largeurs correspondent exactement :
- `swarm_formation.position_command[N]` → `drones.formation_command`
- `drones.position` → `aggregator.positions[N]`

### Broadcast 1 → N
Un seul émetteur, N récepteurs (valeur identique pour tous) :
- `mission_coordinator.mission_status` → `drones.mission_update`
- `aggregator.aggregated_positions` → `drones.neighbor_positions` (after 10 ms)

> **Note LF :** le broadcast 1 → N est sémantiquement valide en Lingua-Franca (1 divise N). Le serveur de langage IDE peut émettre un avertissement pour les largeurs paramétriques ; le compilateur `lfc` compile sans erreur.

### Délai logique (after)
La connexion `aggregated_positions → neighbor_positions` utilise `after 10 ms` pour garantir que chaque drone reçoit les positions agrégées lors d'un instant logique distinct, évitant les cycles causaux.

---

## 8. Schémas

| Fichier | Contenu |
|---------|---------|
| `diagrams/01_system_overview.puml` | Vue d'ensemble : réacteurs et connexions de DroneSwarm |
| `diagrams/02_drone_node_internals.puml` | Structure interne d'un DroneNode (ports + sous-systèmes) |
| `diagrams/03_reactors_interfaces.puml` | Interfaces complètes de tous les réacteurs |
| `diagrams/04_sequence_mission.puml` | Scénario : démarrage mission → obstacle → RTB |
| `diagrams/05_timing.puml` | Diagramme de temporisation des timers |

Pour générer les images PNG à partir des fichiers `.puml` :

```bash
# Avec PlantUML installé localement
plantuml doc/diagrams/*.puml

# Avec Docker
docker run --rm -v $(pwd):/data plantuml/plantuml doc/diagrams/*.puml
```
