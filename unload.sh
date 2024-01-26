#!/usr/bin/env bash
modprobe -r mt7921e
modprobe -r mt7921_common
modprobe -r mt792x_lib
modprobe -r mt76_connac_lib
modprobe -r mt76
modprobe -r iwlmvm
modprobe -r mac80211
