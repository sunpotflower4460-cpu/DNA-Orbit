# DNA Orbit — Claude Code Prompt Library

These prompts are designed for the project foundation in this repository. They state outcomes and boundaries without micromanaging every implementation step.

## 1. Continue autonomously until manual verification

```text
このリポジトリのCLAUDE.md・AGENTS.md・関連Rules/Skills/ADRsを読み、現在のPRと差分を把握してください。

今回の目的は「<目的>」です。

安全で可逆的でローカル検証可能な作業は、質問で止まらず全て自動で進めてください。既存の音・旧プロジェクト互換・パラメータID・リアルタイム安全を守り、物理/数値/心理音響/芸術的拡張を明確に区別してください。

/start-dsp-task の手順で現状と不変条件を整理し、実装・テスト・測定・文書・ADR・敵対的レビューまで進めてください。

最後に、Designed / Implemented / Compiled / Tested / Measured / Listened / Host-validated / Release-ready を分けて報告し、私が手動で確認する項目だけを残してください。
```

## 2. Improve physical fidelity honestly

```text
/physics-audit

<対象機能>について、現在のコードが実際に再現している物理・数値処理・心理音響・芸術的処理を分解してください。

先に数式、単位、座標系、時間基準、仮定、無効になる条件を定義してください。「物理的に正確」という表現は、実装と検証が支える範囲だけに限定してください。

現行方式、最小の正しい基準方式、より高精度な方式、音楽的拡張を比較し、音質・CPU・レイテンシ・自動化・旧state互換を含めた最善案を実装してください。物理精度を上げることで音楽性が下がる場合は、Physical / Musical / Hybridの分離も検討してください。
```

## 3. Maximize sound quality

```text
/audio-quality-gate

<対象機能>の音質を世界基準の商品レベルまで高めてください。ただし、複雑化や高CPUを高音質と見なさないでください。

改善したい聴感を検証可能な主張へ変換し、完全Dry・バイパス・旧互換を先に固定してください。インパルス、スイープ、単音、マルチトーン、逆相、Mid/Side、モノ、実際の音楽素材で測定計画を作り、レベルマッチ試聴とCPU比較まで行ってください。

測定または試聴していないものは「高音質になった」と断定しないでください。
```

## 4. Fix a bug without weakening quality

```text
<症状・再現手順>を修正してください。

まず実際の根本原因を特定し、その原因で失敗する最小の回帰テストを追加してください。テスト許容値を広げる、問題のモードを無効化する、出力ゲインで隠す、旧state移行を削る、という対応は禁止です。

修正後は関連するDSP・状態・ホスト・UI・文書まで横断確認し、/adversarial-review を実行してください。
```

## 5. Optimize CPU without changing sound

```text
現在のReleaseビルドで、同一マシン・同一電源状態・同一入力・同一設定のCPU基準値を先に取得してください。

プロファイルまたはベンチマークでホットスポットを特定し、推測だけで最適化しないでください。変更前後のns/sample/instanceと実時間比を記録し、null差分、周波数/位相、変調副帯波、サンプルレート、複数インスタンス、必要なレベルマッチ試聴で音の不変性を確認してください。

誤差予算がない近似やControl Rate化は採用しないでください。
```

## 6. Add a new parameter or mode

```text
<新機能>を追加してください。

パラメータIDを将来変更しない前提で設計し、値域・単位・スキュー・表示・初期値・自動化・平滑化・プリセット・UI依存状態を決めてください。

旧プロジェクトの音を変えないschema移行値と、新規インスタンスの製品初期値を別々に検討してください。全state round-trip、旧schema fixture、完全プリセット、極端値、自動化、保存途中/復元をテストしてください。
```

## 7. Design a physical mode

```text
DNA Orbitに<物理モードの内容>を設計してください。

現行の心理音響モードを壊さず、Physical / Musical / Hybridを明確に分離してください。ソースと受音点の位置、距離、伝搬遅延、音圧減衰、媒質吸収、指向性、Doppler、出力トポロジー（ステレオ/バイノーラル等）を必要性から選び、未実装の物理を暗黙に一つのフィルターへ押し込まないでください。

まずオフライン基準実装と検証を作り、リアルタイム化は誤差とCPUを測った後に行ってください。
```

## 8. Tune Character or presets scientifically

```text
/experiment-design <比較したいCharacterまたはプリセット案>

音量差で好みが決まらないよう0.1dB以内でレベルマッチしてください。ボーカル、ベース、ドラム、ギター、ピアノ、パッド、シンセ、フルミックスを含め、奥行き、中心安定、低域、幅、過渡、疲労、モノ互換を別々に評価してください。

結果をdocs/experimentsへ残し、採用/却下/要反復/不明を明確にしてください。
```

## 9. Review only, do not edit

```text
この変更を実装者として擁護せず、読み取り専用で敵対的レビューしてください。

physics-auditor、audio-quality-auditor、realtime-safety-reviewer、validation-architectを独立に使い、結論を先に混ぜないでください。

Blocker / High / Medium / Lowで、ファイル・行・再現・破る不変条件・最小修正・再テストを出してください。意図ではなくコードと証拠だけで判定してください。
```

## 10. Prepare for manual DAW test

```text
/release-gate

自動で可能なビルド、CTest、サニタイザー、静的監査、ベンチマーク、スクリーンショット、validator実行を全て完了してください。

その後、私がLUNAまたは他DAWで確認する内容を、1項目ずつ短く、操作手順・期待結果・失敗時に保存する情報付きでまとめてください。自動確認済みの項目を重複して手動依頼しないでください。
```

## 11. Resume after a failed local build

```text
以下はscripts/validate-local.shの出力です。

<ログ>

最初の根本原因から順に修正し、狭い再現コマンドで確認してから全検証を再実行してください。環境不足とコード不具合を分け、未実行の工程を成功扱いしないでください。今回の失敗から将来防げるものはテスト・script・ruleへ残してください。
```

## 12. Final completion request

```text
現在のブランチについて、/adversarial-review と /release-gate を実行してください。

残るBlocker/Highを修正し、自動で可能な全作業を完了してください。最後に、コミット、変更内容、実行コマンド、成功したテスト/測定、未実行の試聴/DAW/署名、既知のリスクを日本語で報告してください。
```
