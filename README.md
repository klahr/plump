# Plump

Plump, the Swedish Oh Hell, for Sailfish OS: against the bots or online.

- Three to five players, each for themselves
- Nineteen deals, from ten cards down to one and back up to ten, with the card after
  the deal turned up for trumps
- Bid exactly the tricks you will take: ten points a trick, ten for a bid of none,
  nothing for a miss; the highest score after the last deal wins
- The dealer bids last and may not make the bids add up to the tricks in the deal
- Bots in three levels: Easy bids by guesswork and plays at random, Medium counts its
  high cards and trumps and plays to its bid, Hard keeps track of who is out of which
  suit and plays every bid and card through to the end of the deal over many guessed deals
- Online play through [cards-server](https://github.com/klahr/cards-server), with bots
  in the empty seats
- Tricks shown at a speed you choose, and won and lost games and bids made per level

Needs Sailfish OS 5.1 or newer, on aarch64 or armv7hl.

## Building

The card code shared with the other card games is the
[sailfish-cards](https://github.com/klahr/sailfish-cards) submodule, so clone with
`git clone --recursive`, or run `git submodule update --init` in an existing clone.

With the [Sailfish SDK](https://docs.sailfishos.org/Tools/Sailfish_SDK/), from the source directory:

```
sfdk -c target=SailfishOS-5.1.0.11-aarch64 build
sfdk -c target=SailfishOS-5.1.0.11-armv7hl build
```

The RPM ends up in `RPMS/`. Clean the build files between targets.

The rules and the bots are plain C++ without Qt, with tests that run on any desktop:

```
make -C tests
```

They also run when the RPM is built.

## License

GPLv3, see [LICENSE](LICENSE).
