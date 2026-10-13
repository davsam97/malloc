# malloc

Réimplémentation de `malloc`, `free`, `realloc` (et `show_alloc_mem`) avec `mmap`/`munmap`.

```sh
make                # -> libft_malloc_$HOSTTYPE.so + lien libft_malloc.so
LD_PRELOAD=./libft_malloc.so ls      # Linux
```

## Conception

| Type  | Taille demandée | Zone                                   |
|-------|-----------------|----------------------------------------|
| TINY  | 1 – 128 o       | 16 Ko (4 pages), au moins 100 blocs    |
| SMALL | 129 – 1024 o    | 104 Ko (26 pages), au moins 100 blocs  |
| LARGE | > 1024 o        | un `mmap` dédié par allocation         |

- Taille des zones : `align_up(header_zone + 100 * (header_bloc + max), pagesize)`
  ([zone/zone.c](srcs/zone/zone.c), `zone_capacity`). C'est donc toujours un multiple de la page.
- Chaque bloc a un en-tête de 32 octets (`size`, `req`, `next`, `free`) et un payload aligné sur 16 octets.
- Les zones sont triées par adresse, donc `show_alloc_mem` affiche par adresses croissantes.
- `free` et `realloc` valident le pointeur (zone connue + bloc exact alloué) : un pointeur invalide est ignoré, et `realloc` renvoie alors `NULL`.
- Une zone TINY/SMALL vide est rendue au système (`munmap`) seulement s'il reste une autre zone du même type.
- `realloc` redimensionne sur place quand c'est possible (réduction, ou absorption du bloc libre suivant).
- 2 globales : `g_heap` (état de l'allocateur) et `g_mutex` (thread-safety).

## Bonus

- **Thread-safe** : `g_mutex` (pthread) protège `malloc`, `free`, `realloc`, `calloc`, `show_alloc_mem` et `show_alloc_mem_ex`.
- **Défragmentation** : à chaque `free`, le bloc fusionne avec ses voisins libres. Les blocs libres sont aussi chaînés dans une free-list par type.
- **`show_alloc_mem_ex()`** : tous les blocs (libres compris), un hexdump des données et l'historique des 64 dernières opérations.
- **Variables d'environnement de debug** :
  - `FT_MALLOC_SCRIBBLE=1` : remplit la mémoire allouée avec `0xAA` et la mémoire libérée avec `0x55`
  - `FT_MALLOC_VERBOSE=1` : trace chaque opération sur stderr
  - `FT_MALLOC_CHECK=1` : signale les pointeurs invalides passés à `free`/`realloc` (`=2` : en plus `abort()`)
- **Compatibilité LD_PRELOAD** : `calloc`, `reallocarray`, `malloc_usable_size`, `posix_memalign`, `aligned_alloc`, `memalign` et `valloc` sont aussi remplacées. Ainsi, aucun pointeur de la glibc ne se mélange aux nôtres.

Fonctions externes : `mmap`, `munmap`, `sysconf`, `getrlimit`, `write`, `pthread_mutex_lock/unlock` ; pour les bonus, `getenv` (variables de debug) et `abort` (`FT_MALLOC_CHECK=2`).
