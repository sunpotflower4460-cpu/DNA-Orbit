# Claude Code フェーズ別プロンプト集

必要なフェーズだけ個別に実行する場合のプロンプトです。

---

# Prompt 0: Baseline監査と基盤

対象リポジトリDNA-Orbitについて、商用化アップグレードのPhase 0を実行してください。

目的:

- 現在の挙動を失わないbaselineを固定
- portability、state、CI、テスト基盤を整える
- DSPの大変更はまだ行わない

必須作業:

1. repo全体の構造とDSP signal flowを監査
2. parameter ID一覧を生成し、変更禁止ルールを文書化
3. state schema versionを追加
4. `M_PI`等の非標準依存を標準C++または既存定数へ置換
5. 現在のCTestを実行
6. baseline audio renderを固定seedで生成
7. baseline screenshotsを生成
8. Linux/macOS/WindowsのCI skeletonを作成
9. sanitizers構成を追加
10. `MANUAL_REQUIRED.md`を作成

禁止:

- 音を大きく変える
- parameter ID rename
- テスト閾値の弱体化
- 未実行検証の完了扱い

終了時に、変更、テスト、未確認事項、次フェーズ準備を報告してください。

---

# Prompt 1: Bypass・Preset・State

DNA-Orbitの商用基盤Phase 1を実行してください。

必須:

1. host bypass中も内部DSP状態を安全に進める
2. processBlockBypassed内のallocationをゼロにする
3. Soft Bypassを50〜100msで実装
4. stale delay、位相停止、復帰clickのテスト追加
5. factory presetを全音響パラメータ定義型に変更
6. preset modified状態を実装
7. legacy state migrationを追加
8. corrupted/future/empty stateの安全テスト
9. editor-only stateとaudio stateを整理
10. README更新

受入基準:

- 10秒bypass後復帰で古い音が出ない
- presetが直前状態に依存しない
- 旧stateが読める
- Mix 0% dryを壊さない
- CTest全通過

---

# Prompt 2: Stereo Preserve DSP

DNA-Orbitの最重要DSP改善を実行してください。

現在のWetが`0.5*(L+R)`由来でStereo Sideを失う問題を解消します。

必須:

1. M/S分解
2. `stereoPreserve`パラメータ追加
3. `sourceA = M + pS`
4. `sourceB = M - pS`
5. p=0で旧挙動に近い
6. p=1でA=L、B=R
7. CoreもStereo Preserveに応じてL/R情報を保持
8. legacy stateではp=0
9. 新規instanceでは適切なdefault
10. Auto Gainモデルへ反映
11. anti-phase、uncorrelated、L-only、R-onlyテスト
12. CPU回帰測定

受入基準:

- HELIX通常モードで`L=-R`入力が不意に無音にならない
- Preserveの変化が連続
- NULL COREの意図したmono collapseは維持
- 既存state互換
- finite
- clickなし

---

# Prompt 3: Bass Anchor・Mix Law・Character

DNA-Orbitへ音楽制作向けの中核品質を追加してください。

必須:

1. 4th-order Linkwitz–Riley Bass Anchor
2. `bassAnchorHz`
3. low bandを安定したoriginal/core pathへ
4. high bandをorbit
5. 再合成の周波数応答テスト
6. Natural/Vivid/Deep Character
7. high shelf/LPF/delay mappingの整理
8. Wet compensationとDry/Wet Mix Lawの分離
9. Dry/Wet相関込みのレベル補正
10. 補正上限とsilence安全
11. Mix 0→100→0テスト
12. Control Rate最適化

受入基準:

- crossover再合成が測定上フラット
- 低域定位が安定
- 一般素材でMix sweepの長期RMSが目標範囲
- Radius 0/Depth 0/Twist 0で大きな+3dB膨張を抑制
- pumpingを避ける
- CPUが大幅悪化しない

---

# Prompt 4: Host Phase Lock

DNA-Orbitへ再現可能なテンポ・位相同期を実装してください。

必須:

1. Free/Retrigger/Host Lock
2. `phaseMode`
3. `startPhase`
4. `direction`
5. BPM、PPQ、time signature
6. 3/4、4/4、6/8対応
7. PPQ jumpとloop
8. Host tempo unavailable fallback
9. same PPQ same phase
10. offline determinism
11. position jump時のclick抑制
12. UIへmode状態を公開

受入基準:

- 同じプロジェクト位置から同じ軌道
- リアルタイムとオフラインの一致
- loopで意図した繰り返し
- stop/playでモード通り
- transport情報欠落でも安全

---

# Prompt 5: UI/UX再設計

DNA-OrbitのUI/UXを世界基準の商用製品へ改善してください。

設計原則:

- 生命科学と宇宙観測が融合した静かな未来の楽器
- 3D DNAは実DSP状態を示す
- Basicは初心者がすぐ良い音へ到達
- Detailは専門家が理由を理解して調整
- 危険状態を隠さない

必須:

1. Header: preset/A-B/undo/redo/bypass/settings
2. Basic: Motion/Width/Depth/Core/Mix
3. Detail: Motion/Space/Mix
4. 3D direct manipulation
5. Sync中のRate状態
6. NULL CORE中のCore disabled
7. Mono Preview
8. 全parameter host automation同期
9. Japanese/English
10. keyboard操作
11. screen reader名称
12. reduced motion
13. DPI/responsive
14. headless screenshots
15. editor hidden時timer停止

受入基準:

- 780×540〜1600×1100
- 文字切れなし
- 色だけに依存しない
- keyboard-only
- host automationと表示一致
- UI CPU予算内

---

# Prompt 6: QA・Release Candidate

DNA-OrbitをRelease Candidateへ仕上げてください。

必須:

1. CTest全通過
2. pluginval strictness 10
3. VST3 Validator
4. auval
5. ASan/UBSan
6. Windows/macOS/Linux CI
7. golden audio regression
8. screenshot regression
9. performance report
10. license audit
11. macOS signing/notarization workflow
12. Windows signing/installer workflow
13. release checklist
14. clean machine手順
15. manual listening checklist
16. known limitations

実行不能な項目は、具体的コマンドと期待結果を`MANUAL_REQUIRED.md`へ記載してください。

最終報告では:

- 自動完了
- 実機のみ
- 未解決
- release blocker
- 推奨version

を明確に分けてください。

---

# Prompt 7: 最終コードレビュー

DNA-Orbitの現在のHEADを、世界基準の商用出荷直前として敵対的にレビューしてください。

観点:

- audio thread
- allocation
- race
- state migration
- parameter automation
- bypass
- transport
- stereo/mono
- anti-phase
- level
- CPU
- UI lifecycle
- localization
- validation
- packaging
- licensing

単なる指摘ではなく:

1. severity
2. evidence
3. reproduction
4. fix
5. test
6. release blocker判定

を出してください。

自動で安全に修正可能なものは修正し、テストまで実行してください。
