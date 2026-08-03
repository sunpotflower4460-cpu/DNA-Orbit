# DNA Orbit 手動試聴・DAW検証仕様書

自動テストで「壊れていない」は証明できても、「音楽的に美しい」は最終的に耳で確認する必要がある。

この工程は、コード側で可能な作業を完了した後に行う。

---

# 1. 試聴環境

最低限:

- ヘッドホン
- ステレオスピーカー
- モノラル確認
- 小音量
- 通常音量

可能なら:

- イヤホン
- Bluetoothスピーカー
- スマートフォン相当の小型再生
- サブウーファーあり/なし

---

# 2. DAW

優先:

1. Cubase
2. Logic Pro
3. Ableton Live
4. REAPER
5. Studio One
6. FL Studio

自分の制作環境ではCubaseを最優先で確認する。

---

# 3. 素材セット

各素材はDryの音量を記録し、同じ部分をループする。

## Mono

- 男声ボーカル
- 女声ボーカル
- アコースティックギター
- エレキギター
- リードシンセ
- ベース

## Stereo

- ピアノ
- パッド
- コーラス
- ドラムループ
- ステレオギター
- アンビエントテクスチャ
- 完成ミックス

## Stress

- L only
- R only
- anti-phase
- extremely wide
- transient click
- low sine
- high sine
- pink noise

---

# 4. 基本試聴

各素材で:

1. Mix 0%
2. Mix 30%
3. Mix 50%
4. Mix 100%
5. bypass
6. level matched bypass

評価:

- 音量が上がっただけで良く聞こえていないか
- centerが痩せないか
- low endが揺れないか
- 高域が曇りすぎないか
- 左右往復にしか聞こえないか
- 前後感が自然か
- コーム感が音楽的か
- transientがぼやけないか
- 速いMotionで酔わないか

---

# 5. Stereo Preserve試聴

0/25/50/75/100%。

確認:

- 0%: 中心安定、旧挙動
- 50〜75%: 多くの素材で自然
- 100%: 元のL/Rを保ちながら動く
- wide padで中央へ潰れない
- anti-phaseで通常HELIXが消えない
- mono sumで過度に痩せない

Default候補を素材全体で決める。

---

# 6. Bass Anchor試聴

Off/80/120/180/250Hz。

確認:

- キックの位置
- ベースの芯
- 低音の位相
- full mixの安定
- cutoff付近の違和感
- 低域だけ別物に聞こえないか

Default候補は120Hzを基準とする。

---

# 7. Character試聴

Natural/Vivid/Deep。

### Natural

- ボーカル
- ギター
- full mix
- 長時間でも疲れない

### Vivid

- synth
- pad
- chorus
- motionが明確

### Deep

- ambient
- sound design
- 背後感
- 音色が暗くなりすぎない

数値調整は、単独で派手かではなくミックス内で判断する。

---

# 8. Mix/Auto Level

Mixを0→100へ8〜16小節でautomation。

確認:

- 中間で膨らまない
- 100%で急に小さくならない
- pumping
- transientで補正が揺れない
- silence後の復帰
- Auto Level OFFとの差

A/Bは必ずラウドネスマッチで行う。

---

# 9. Transport

- Play
- Stop
- 途中再生
- Loop
- Tempo change
- Project reopen
- Offline bounce

モード:

- Free
- Retrigger
- Host Lock

確認:

- Host Lockで毎回同じ位置
- Loopの頭で同じ軌道
- Retriggerが意図通り
- Freeが不自然にリセットされない
- offline bounceと一致

---

# 10. Bypass

- 1秒
- 10秒
- 30秒
- 高速連打
- automation

確認:

- 古いDelay音
- click
- 位相飛び
- 音量ジャンプ
- visual状態
- host bypassとsoft bypassの違い

---

# 11. NULL CORE

- Mix 30%
- Mix 100%
- Safety 0/10/30%
- Mono Preview

確認:

- 通常モードとの違いが明確
- モノ危険が理解できる
- Core disabledが分かる
- 意図しない爆音補正なし
- sound designとして魅力がある

---

# 12. プリセット評価表

各presetを5点満点で評価。

| 項目 | 点数 |
|---|---:|
| 名前と音が一致 | /5 |
| 挿した瞬間に価値が分かる | /5 |
| 音量が公平 | /5 |
| モノ安全 | /5 |
| low end | /5 |
| original character保持 | /5 |
| commercial usability | /5 |

平均4未満は再調整。

---

# 13. 最終合格基準

- Basicだけで30秒以内に良い音
- Vocal presetでcenterが失われない
- Pad presetで単純なauto-pan以上
- Guitar presetでコーム感が不快でない
- Bass Anchorで低域が安定
- Host Lockが再現可能
- Mix automationで音量が暴れない
- bypass復帰が自然
- HELIXは通常monoで消えない
- NULL COREだけが意図的に危険
- CPUが制作を妨げない
- 3D表示が音の理解を助ける

---

# 14. 記録テンプレート

```text
DAW:
OS:
Plugin format:
Sample rate:
Buffer:
Source:
Preset:
Parameters:

良かった点:
問題:
再現手順:
重要度:
推奨修正:
修正後確認:
```
