#!/usr/bin/env python3
"""Write menu/pocket_menu_intl.h: the Pocket Menu strings for every
RetroArch language, one array per language indexed by
(id - MENU_ENUM_POCKET_FIRST). A NULL slot falls back to msg_hash_us.h.

Usage, from the repository root:
    python3 tools/gen_pocket_intl.py

The strings live here, not in the Crowdin-generated intl/msg_hash_*.h
tables, so an upstream translation sync never touches them.
"""
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Same order as the block in msg_hash.h. MENU_LABEL(X) expands to
# LABEL, SUBLABEL, LABEL_VALUE.
LABELS = ["MENU", "SAVE_STATE", "LOAD_STATE", "STATE_SLOT", "PERF_PROFILE",
          "PERF_VALUE", "SAVE_PERF_CORE", "SAVE_PERF_ROM", "JOYPAD_SENSE",
          "SETTINGS", "RETURN"]
EXTRA = ["VALUE_PERF_SAVER", "VALUE_PERF_BALANCED", "VALUE_PERF_MAX",
         "VALUE_SENSE_OFF", "VALUE_SENSE_FAST", "VALUE_SENSE_SLOW",
         "VALUE_SLOT", "VALUE_SLOT_EMPTY", "VALUE_NOT_SAVED",
         "MSG_PERF_SAVED", "MSG_PERF_SAVE_FAILED"]

IDS = []
for l in LABELS:
    IDS += ["LABEL_" + l, "SUBLABEL_" + l, "VALUE_" + l]
IDS += EXTRA

# Short keys used below -> id
K = {
    "menu": "VALUE_MENU",
    "save": "VALUE_SAVE_STATE", "save_sub": "SUBLABEL_SAVE_STATE",
    "load": "VALUE_LOAD_STATE", "load_sub": "SUBLABEL_LOAD_STATE",
    "slot_sub": "SUBLABEL_STATE_SLOT",
    "perf": "VALUE_PERF_PROFILE", "perf_sub": "SUBLABEL_PERF_PROFILE",
    "perf_core": "VALUE_SAVE_PERF_CORE", "perf_core_sub": "SUBLABEL_SAVE_PERF_CORE",
    "perf_rom": "VALUE_SAVE_PERF_ROM", "perf_rom_sub": "SUBLABEL_SAVE_PERF_ROM",
    "sense": "VALUE_JOYPAD_SENSE", "sense_sub": "SUBLABEL_JOYPAD_SENSE",
    "settings": "VALUE_SETTINGS", "settings_sub": "SUBLABEL_SETTINGS",
    "ret": "VALUE_RETURN", "ret_sub": "SUBLABEL_RETURN",
    "saver": "VALUE_PERF_SAVER", "balanced": "VALUE_PERF_BALANCED", "max": "VALUE_PERF_MAX",
    "off": "VALUE_SENSE_OFF", "fast": "VALUE_SENSE_FAST", "slow": "VALUE_SENSE_SLOW",
    "slot": "VALUE_SLOT", "empty": "VALUE_SLOT_EMPTY", "na": "VALUE_NOT_SAVED",
    "saved": "MSG_PERF_SAVED", "failed": "MSG_PERF_SAVE_FAILED",
}

# Short names reused from the Miyoo Menu translations
# (/opt/codes/RetroArch intl, Rparadise-Team fork): Miyoo key -> our key.
MIYOO_REUSE = {
    "MENU_ENUM_LABEL_VALUE_MIYOO_SAVE_STATE": "save",
    "MENU_ENUM_LABEL_VALUE_MIYOO_LOAD_STATE": "load",
    "MENU_ENUM_LABEL_VALUE_MIYOO_JOYPAD_SENSE": "sense",
    "MENU_ENUM_LABEL_VALUE_MIYOO_JOYPAD_SENSE_OFF": "off",
    "MENU_ENUM_LABEL_VALUE_MIYOO_JOYPAD_SENSE_FAST": "fast",
    "MENU_ENUM_LABEL_VALUE_MIYOO_JOYPAD_SENSE_SLOW": "slow",
    "MENU_ENUM_LABEL_VALUE_MIYOO_RETROARCH_SETTINGS": "settings",
}
MIYOO_INTL = "/opt/codes/RetroArch/intl"

# Hand-written strings for this port. Keys missing here fall back to
# the Miyoo short name (if any) and then to English.
NEW = {
 "es": dict(save_sub="Elige una ranura para guardar la partida con una captura del momento.",
   load_sub="Elige una ranura para cargar, con vista previa de su captura.",
   slot_sub="Pulsa para usar esta ranura y volver al juego.",
   perf="Perfil de rendimiento",
   perf_sub="Ahorro ejecuta la emulación en los núcleos de eficiencia. Equilibrado lo deja al sistema. Máximo la mantiene en los núcleos más rápidos y pide rendimiento sostenido.",
   perf_core="Guardar perfil (núcleo)", perf_core_sub="Usar el perfil actual cada vez que arranque este núcleo.",
   perf_rom="Guardar perfil (juego)", perf_rom_sub="Usar el perfil actual para este juego. Tiene prioridad sobre el del núcleo.",
   sense="Sensibilidad del mando",
   sense_sub="Margen para reconocer botones de acción pulsados a la vez. Instantáneo no añade retardo; Rápido espera 1 frame; Lento espera 2.",
   off="Instantáneo", fast="Rápido", slow="Lento",
   settings_sub="Abre el Menú rápido completo de RetroArch para opciones avanzadas.",
   ret_sub="Volver al Pocket Menu.",
   saver="Ahorro", balanced="Equilibrado", max="Máximo",
   slot="Ranura %d", empty="Vacía", na="N/D",
   saved="Perfil de rendimiento guardado: %s (%s)", failed="No se pudo guardar el perfil de rendimiento."),
 "ast": dict(perf="Perfil de rendimientu", saver="Aforru", balanced="Equilibráu", max="Máximu",
   perf_core="Guardar perfil (núcleu)", perf_rom="Guardar perfil (xuegu)", slot="Ranura %d", empty="Balera", na="N/D"),
 "ca": dict(save_sub="Tria una ranura per desar la partida amb una captura del moment.",
   load_sub="Tria una ranura per carregar, amb la previsualització de la captura.",
   slot_sub="Prem per fer servir aquesta ranura i tornar al joc.",
   perf="Perfil de rendiment",
   perf_sub="Estalvi executa l'emulació als nuclis d'eficiència. Equilibrat ho deixa al sistema. Màxim la manté als nuclis més ràpids i demana rendiment sostingut.",
   perf_core="Desar perfil (nucli)", perf_core_sub="Fes servir el perfil actual cada cop que s'iniciï aquest nucli.",
   perf_rom="Desar perfil (joc)", perf_rom_sub="Fes servir el perfil actual per a aquest joc. Té prioritat sobre el del nucli.",
   sense_sub="Marge per reconèixer botons d'acció premuts alhora. Instantani no afegeix retard; Ràpid espera 1 frame; Lent n'espera 2.",
   settings_sub="Obre el menú ràpid complet de RetroArch per a opcions avançades.",
   ret_sub="Torna al Pocket Menu.", saver="Estalvi", balanced="Equilibrat", max="Màxim",
   slot="Ranura %d", empty="Buida", na="N/D",
   saved="Perfil de rendiment desat: %s (%s)", failed="No s'ha pogut desar el perfil de rendiment."),
 "gl": dict(save_sub="Escolle unha rañura para gardar a partida cunha captura do momento.",
   load_sub="Escolle unha rañura para cargar, coa vista previa da captura.",
   slot_sub="Preme para usar esta rañura e volver ao xogo.",
   perf="Perfil de rendemento",
   perf_sub="Aforro executa a emulación nos núcleos de eficiencia. Equilibrado déixao ao sistema. Máximo mantena nos núcleos máis rápidos e pide rendemento sostido.",
   perf_core="Gardar perfil (núcleo)", perf_core_sub="Usar o perfil actual cada vez que arrinque este núcleo.",
   perf_rom="Gardar perfil (xogo)", perf_rom_sub="Usar o perfil actual para este xogo. Ten prioridade sobre o do núcleo.",
   settings_sub="Abre o menú rápido completo de RetroArch para opcións avanzadas.",
   ret_sub="Volver ao Pocket Menu.", saver="Aforro", balanced="Equilibrado", max="Máximo",
   slot="Rañura %d", empty="Baleira", na="N/D",
   saved="Perfil de rendemento gardado: %s (%s)", failed="Non se puido gardar o perfil de rendemento."),
 "fr": dict(save_sub="Choisissez un emplacement pour sauvegarder la partie avec une capture du moment.",
   load_sub="Choisissez un emplacement à charger, avec l'aperçu de sa capture.",
   slot_sub="Appuyez pour utiliser cet emplacement et revenir au jeu.",
   perf="Profil de performances",
   perf_sub="Économie exécute l'émulation sur les cœurs basse consommation. Équilibré laisse faire le système. Maximum la garde sur les cœurs les plus rapides et demande des performances soutenues.",
   perf_core="Enregistrer le profil (cœur)", perf_core_sub="Utiliser le profil actuel à chaque démarrage de ce cœur.",
   perf_rom="Enregistrer le profil (jeu)", perf_rom_sub="Utiliser le profil actuel pour ce jeu. Prioritaire sur celui du cœur.",
   sense_sub="Délai pour reconnaître les boutons d'action pressés ensemble. Instantané n'ajoute aucun délai ; Rapide attend 1 image ; Lent en attend 2.",
   settings_sub="Ouvre le menu rapide complet de RetroArch pour les options avancées.",
   ret_sub="Revenir au Pocket Menu.", saver="Économie", balanced="Équilibré", max="Maximum",
   slot="Emplacement %d", empty="Vide", na="N/D",
   saved="Profil de performances enregistré : %s (%s)", failed="Impossible d'enregistrer le profil de performances."),
 "de": dict(save_sub="Wähle einen Platz, um das Spiel mit einem Bildschirmfoto zu speichern.",
   load_sub="Wähle einen Platz zum Laden, mit Vorschau des Bildschirmfotos.",
   slot_sub="Drücken, um diesen Platz zu verwenden und zum Spiel zurückzukehren.",
   perf="Leistungsprofil",
   perf_sub="Sparen führt die Emulation auf den Effizienzkernen aus. Ausgewogen überlässt es dem System. Maximum hält sie auf den schnellsten Kernen und fordert Dauerleistung an.",
   perf_core="Profil speichern (Core)", perf_core_sub="Das aktuelle Profil bei jedem Start dieses Cores verwenden.",
   perf_rom="Profil speichern (Spiel)", perf_rom_sub="Das aktuelle Profil für dieses Spiel verwenden. Hat Vorrang vor dem Core-Profil.",
   sense_sub="Zeitfenster, um gleichzeitig gedrückte Aktionstasten zu erkennen. Sofort fügt keine Verzögerung hinzu; Schnell wartet 1 Frame; Langsam 2.",
   settings_sub="Öffnet das vollständige Schnellmenü von RetroArch für erweiterte Optionen.",
   ret_sub="Zurück zum Pocket Menu.", saver="Sparen", balanced="Ausgewogen", max="Maximum",
   slot="Platz %d", empty="Leer", na="k. A.",
   saved="Leistungsprofil gespeichert: %s (%s)", failed="Leistungsprofil konnte nicht gespeichert werden."),
 "it": dict(save_sub="Scegli uno slot per salvare la partita con uno screenshot del momento.",
   load_sub="Scegli uno slot da caricare, con l'anteprima dello screenshot.",
   slot_sub="Premi per usare questo slot e tornare al gioco.",
   perf="Profilo prestazioni",
   perf_sub="Risparmio esegue l'emulazione sui core a basso consumo. Bilanciato lascia decidere al sistema. Massimo la tiene sui core più veloci e richiede prestazioni sostenute.",
   perf_core="Salva profilo (core)", perf_core_sub="Usa il profilo attuale ogni volta che si avvia questo core.",
   perf_rom="Salva profilo (gioco)", perf_rom_sub="Usa il profilo attuale per questo gioco. Ha la priorità su quello del core.",
   sense_sub="Margine per riconoscere i pulsanti d'azione premuti insieme. Istantaneo non aggiunge ritardo; Veloce attende 1 frame; Lento 2.",
   settings_sub="Apre il menu rapido completo di RetroArch per le opzioni avanzate.",
   ret_sub="Torna al Pocket Menu.", saver="Risparmio", balanced="Bilanciato", max="Massimo",
   slot="Slot %d", empty="Vuoto", na="N/D",
   saved="Profilo prestazioni salvato: %s (%s)", failed="Impossibile salvare il profilo prestazioni."),
 "pt_br": dict(save_sub="Escolha um slot para salvar o jogo com uma captura do momento.",
   load_sub="Escolha um slot para carregar, com a prévia da captura.",
   slot_sub="Pressione para usar este slot e voltar ao jogo.",
   perf="Perfil de desempenho",
   perf_sub="Economia executa a emulação nos núcleos de eficiência. Equilibrado deixa com o sistema. Máximo a mantém nos núcleos mais rápidos e pede desempenho sustentado.",
   perf_core="Salvar perfil (núcleo)", perf_core_sub="Usar o perfil atual sempre que este núcleo iniciar.",
   perf_rom="Salvar perfil (jogo)", perf_rom_sub="Usar o perfil atual para este jogo. Tem prioridade sobre o do núcleo.",
   sense_sub="Margem para reconhecer botões de ação pressionados juntos. Instantâneo não adiciona atraso; Rápido espera 1 quadro; Lento espera 2.",
   settings_sub="Abre o Menu rápido completo do RetroArch para opções avançadas.",
   ret_sub="Voltar ao Pocket Menu.", saver="Economia", balanced="Equilibrado", max="Máximo",
   slot="Slot %d", empty="Vazio", na="N/D",
   saved="Perfil de desempenho salvo: %s (%s)", failed="Não foi possível salvar o perfil de desempenho."),
 "pt_pt": dict(save_sub="Escolhe uma ranhura para gravar o jogo com uma captura do momento.",
   load_sub="Escolhe uma ranhura para carregar, com a pré-visualização da captura.",
   slot_sub="Prime para usar esta ranhura e voltar ao jogo.",
   perf="Perfil de desempenho",
   perf_sub="Poupança executa a emulação nos núcleos de eficiência. Equilibrado deixa ao sistema. Máximo mantém-na nos núcleos mais rápidos e pede desempenho sustentado.",
   perf_core="Gravar perfil (núcleo)", perf_core_sub="Usar o perfil atual sempre que este núcleo arrancar.",
   perf_rom="Gravar perfil (jogo)", perf_rom_sub="Usar o perfil atual para este jogo. Tem prioridade sobre o do núcleo.",
   settings_sub="Abre o Menu rápido completo do RetroArch para opções avançadas.",
   ret_sub="Voltar ao Pocket Menu.", saver="Poupança", balanced="Equilibrado", max="Máximo",
   slot="Ranhura %d", empty="Vazia", na="N/D",
   saved="Perfil de desempenho gravado: %s (%s)", failed="Não foi possível gravar o perfil de desempenho."),
 "nl": dict(perf="Prestatieprofiel", perf_core="Profiel opslaan (core)", perf_rom="Profiel opslaan (spel)",
   perf_sub="Besparing draait de emulatie op de zuinige kernen. Gebalanceerd laat het aan het systeem over. Maximaal houdt haar op de snelste kernen en vraagt om aanhoudende prestaties.",
   saver="Besparing", balanced="Gebalanceerd", max="Maximaal", slot="Slot %d", empty="Leeg", na="n.v.t.",
   ret_sub="Terug naar het Pocket Menu.", saved="Prestatieprofiel opgeslagen: %s (%s)", failed="Kan het prestatieprofiel niet opslaan."),
 "pl": dict(perf="Profil wydajności", perf_core="Zapisz profil (rdzeń)", perf_rom="Zapisz profil (gra)",
   perf_sub="Oszczędzanie uruchamia emulację na rdzeniach energooszczędnych. Zrównoważony zostawia to systemowi. Maksymalny trzyma ją na najszybszych rdzeniach i prosi o stałą wydajność.",
   saver="Oszczędzanie", balanced="Zrównoważony", max="Maksymalny", slot="Slot %d", empty="Pusty", na="b.d.",
   ret_sub="Powrót do Pocket Menu.", saved="Zapisano profil wydajności: %s (%s)", failed="Nie można zapisać profilu wydajności."),
 "ru": dict(save_sub="Выберите слот, чтобы сохранить игру со снимком момента.",
   load_sub="Выберите слот для загрузки с предпросмотром снимка.",
   slot_sub="Нажмите, чтобы использовать этот слот и вернуться в игру.",
   perf="Профиль производительности",
   perf_sub="Экономия запускает эмуляцию на энергоэффективных ядрах. Баланс оставляет выбор системе. Максимум держит её на самых быстрых ядрах и запрашивает устойчивую производительность.",
   perf_core="Сохранить профиль (ядро)", perf_core_sub="Использовать текущий профиль при каждом запуске этого ядра.",
   perf_rom="Сохранить профиль (игра)", perf_rom_sub="Использовать текущий профиль для этой игры. Имеет приоритет над профилем ядра.",
   settings_sub="Открыть полное быстрое меню RetroArch с расширенными настройками.",
   ret_sub="Вернуться в Pocket Menu.", saver="Экономия", balanced="Баланс", max="Максимум",
   slot="Слот %d", empty="Пусто", na="Н/Д",
   saved="Профиль производительности сохранён: %s (%s)", failed="Не удалось сохранить профиль производительности."),
 "uk": dict(perf="Профіль продуктивності", perf_core="Зберегти профіль (ядро)", perf_rom="Зберегти профіль (гра)",
   saver="Економія", balanced="Баланс", max="Максимум", slot="Слот %d", empty="Порожньо", na="Н/Д",
   ret_sub="Повернутися до Pocket Menu.", saved="Профіль продуктивності збережено: %s (%s)", failed="Не вдалося зберегти профіль продуктивності."),
 "be": dict(perf="Профіль прадукцыйнасці", saver="Эканомія", balanced="Баланс", max="Максімум", slot="Слот %d", empty="Пуста"),
 "tr": dict(perf="Performans profili", perf_core="Profili kaydet (çekirdek)", perf_rom="Profili kaydet (oyun)",
   perf_sub="Tasarruf, emülasyonu verimlilik çekirdeklerinde çalıştırır. Dengeli kararı sisteme bırakır. Maksimum onu en hızlı çekirdeklerde tutar ve sürekli performans ister.",
   saver="Tasarruf", balanced="Dengeli", max="Maksimum", slot="Yuva %d", empty="Boş", na="Yok",
   ret_sub="Pocket Menu'ye dön.", saved="Performans profili kaydedildi: %s (%s)", failed="Performans profili kaydedilemedi."),
 "ja": dict(save_sub="スロットを選んで、その瞬間のスクリーンショット付きでセーブします。",
   load_sub="スクリーンショットを確認しながらロードするスロットを選びます。",
   slot_sub="このスロットを使ってゲームに戻ります。",
   perf="パフォーマンスプロファイル",
   perf_sub="省電力は高効率コアでエミュレーションを実行します。バランスはシステムに任せます。最大は最速のコアで実行し、持続的なパフォーマンスを要求します。",
   perf_core="プロファイルを保存 (コア)", perf_core_sub="このコアを起動するたびに現在のプロファイルを使用します。",
   perf_rom="プロファイルを保存 (ゲーム)", perf_rom_sub="このゲームで現在のプロファイルを使用します。コアのプロファイルより優先されます。",
   settings_sub="詳細設定用に RetroArch のクイックメニューを開きます。",
   ret_sub="Pocket Menu に戻ります。", saver="省電力", balanced="バランス", max="最大",
   slot="スロット %d", empty="空き", na="なし",
   saved="パフォーマンスプロファイルを保存しました: %s (%s)", failed="パフォーマンスプロファイルを保存できませんでした。"),
 "ko": dict(save_sub="슬롯을 골라 그 순간의 스크린샷과 함께 저장합니다.",
   load_sub="스크린샷을 미리 보며 불러올 슬롯을 고릅니다.",
   slot_sub="이 슬롯을 사용하고 게임으로 돌아갑니다.",
   perf="성능 프로필",
   perf_sub="절전은 고효율 코어에서 에뮬레이션을 실행합니다. 균형은 시스템에 맡깁니다. 최대는 가장 빠른 코어에서 실행하고 지속 성능을 요청합니다.",
   perf_core="프로필 저장 (코어)", perf_core_sub="이 코어를 시작할 때마다 현재 프로필을 사용합니다.",
   perf_rom="프로필 저장 (게임)", perf_rom_sub="이 게임에 현재 프로필을 사용합니다. 코어 프로필보다 우선합니다.",
   settings_sub="고급 옵션을 위해 RetroArch 빠른 메뉴를 엽니다.",
   ret_sub="Pocket Menu로 돌아갑니다.", saver="절전", balanced="균형", max="최대",
   slot="슬롯 %d", empty="비어 있음", na="없음",
   saved="성능 프로필 저장됨: %s (%s)", failed="성능 프로필을 저장할 수 없습니다."),
 "chs": dict(save_sub="选择一个存档位，保存游戏并附带当时的截图。",
   load_sub="选择要读取的存档位，可预览其截图。",
   slot_sub="使用此存档位并返回游戏。",
   perf="性能配置",
   perf_sub="省电在能效核心上运行模拟。均衡交由系统决定。最高将其保持在最快的核心上并请求持续性能。",
   perf_core="保存配置（核心）", perf_core_sub="每次启动此核心时使用当前配置。",
   perf_rom="保存配置（游戏）", perf_rom_sub="为此游戏使用当前配置，优先于核心配置。",
   settings_sub="打开完整的 RetroArch 快捷菜单以使用高级选项。",
   ret_sub="返回 Pocket Menu。", saver="省电", balanced="均衡", max="最高",
   slot="存档位 %d", empty="空", na="无",
   saved="已保存性能配置：%s（%s）", failed="无法保存性能配置。"),
 "cht": dict(save_sub="選擇一個存檔位，儲存遊戲並附帶當時的截圖。",
   load_sub="選擇要讀取的存檔位，可預覽其截圖。",
   slot_sub="使用此存檔位並返回遊戲。",
   perf="效能設定檔",
   perf_sub="省電在節能核心上執行模擬。平衡交由系統決定。最高將其保持在最快的核心上並要求持續效能。",
   perf_core="儲存設定檔（核心）", perf_core_sub="每次啟動此核心時使用目前的設定檔。",
   perf_rom="儲存設定檔（遊戲）", perf_rom_sub="為此遊戲使用目前的設定檔，優先於核心設定檔。",
   settings_sub="開啟完整的 RetroArch 快速選單以使用進階選項。",
   ret_sub="返回 Pocket Menu。", saver="省電", balanced="平衡", max="最高",
   slot="存檔位 %d", empty="空", na="無",
   saved="已儲存效能設定檔：%s（%s）", failed="無法儲存效能設定檔。"),
 "cs": dict(perf="Profil výkonu", saver="Úspora", balanced="Vyvážený", max="Maximum", slot="Pozice %d", empty="Prázdná"),
 "sk": dict(perf="Profil výkonu", saver="Úspora", balanced="Vyvážený", max="Maximum", slot="Pozícia %d", empty="Prázdna"),
 "hu": dict(perf="Teljesítményprofil", saver="Takarékos", balanced="Kiegyensúlyozott", max="Maximális", slot="%d. hely", empty="Üres"),
 "fi": dict(perf="Suorituskykyprofiili", saver="Virransäästö", balanced="Tasapainoinen", max="Maksimi", slot="Paikka %d", empty="Tyhjä"),
 "sv": dict(perf="Prestandaprofil", saver="Energisparläge", balanced="Balanserad", max="Maximal", slot="Plats %d", empty="Tom"),
 "no": dict(perf="Ytelsesprofil", saver="Strømsparing", balanced="Balansert", max="Maksimal", slot="Plass %d", empty="Tom"),
 "id": dict(perf="Profil kinerja", saver="Hemat daya", balanced="Seimbang", max="Maksimum", slot="Slot %d", empty="Kosong"),
 "vn": dict(perf="Cấu hình hiệu năng", saver="Tiết kiệm", balanced="Cân bằng", max="Tối đa", slot="Ô %d", empty="Trống"),
 "el": dict(perf="Προφίλ επιδόσεων", saver="Εξοικονόμηση", balanced="Ισορροπημένο", max="Μέγιστο", slot="Θέση %d", empty="Κενή"),
 "ar": dict(perf="ملف الأداء", saver="توفير الطاقة", balanced="متوازن", max="أقصى", slot="الخانة %d", empty="فارغة"),
 "he": dict(perf="פרופיל ביצועים", saver="חיסכון", balanced="מאוזן", max="מרבי", slot="משבצת %d", empty="ריקה"),
 "fa": dict(perf="نمایه کارایی", saver="صرفه‌جویی", balanced="متعادل", max="حداکثر", slot="جایگاه %d", empty="خالی"),
 "eo": dict(perf="Rendimenta profilo", saver="Ŝparado", balanced="Ekvilibra", max="Maksimuma", slot="Ingo %d", empty="Malplena"),
 "ga": dict(perf="Próifíl feidhmíochta", saver="Coigilt", balanced="Cothromaithe", max="Uasta", slot="Sliotán %d", empty="Folamh"),
 "th": dict(perf="โปรไฟล์ประสิทธิภาพ", saver="ประหยัดพลังงาน", balanced="สมดุล", max="สูงสุด", slot="ช่อง %d", empty="ว่าง"),
}
NEW["val"] = dict(NEW["ca"])

# file suffix -> RETRO_LANGUAGE_* (same table msg_hash.c switches on)
LANGS = [
 ("fr", "FRENCH"), ("de", "GERMAN"), ("es", "SPANISH"), ("it", "ITALIAN"),
 ("pt_br", "PORTUGUESE_BRAZIL"), ("pt_pt", "PORTUGUESE_PORTUGAL"), ("nl", "DUTCH"),
 ("eo", "ESPERANTO"), ("pl", "POLISH"), ("ru", "RUSSIAN"), ("ja", "JAPANESE"),
 ("ko", "KOREAN"), ("vn", "VIETNAMESE"), ("chs", "CHINESE_SIMPLIFIED"),
 ("cht", "CHINESE_TRADITIONAL"), ("ar", "ARABIC"), ("el", "GREEK"), ("tr", "TURKISH"),
 ("sk", "SLOVAK"), ("fa", "PERSIAN"), ("he", "HEBREW"), ("ast", "ASTURIAN"),
 ("fi", "FINNISH"), ("id", "INDONESIAN"), ("sv", "SWEDISH"), ("uk", "UKRAINIAN"),
 ("cs", "CZECH"), ("val", "CATALAN_VALENCIA"), ("ca", "CATALAN"),
 ("hu", "HUNGARIAN"), ("be", "BELARUSIAN"), ("gl", "GALICIAN"), ("no", "NORWEGIAN"),
 ("ga", "IRISH"), ("th", "THAI"),
]

ROW = re.compile(r'MSG_HASH\(\s*(\w+)\s*,\s*"((?:[^"\\]|\\.)*)"\s*\)', re.S)


def miyoo_strings(suffix):
    path = os.path.join(MIYOO_INTL, "msg_hash_%s.h" % suffix)
    out = {}
    if not os.path.isfile(path):
        return out
    with open(path, encoding="utf-8") as f:
        text = f.read()
    for key, val in ROW.findall(text):
        if key in MIYOO_REUSE and val.strip():
            out[MIYOO_REUSE[key]] = val
    return out


def c_escape(s):
    out = []
    for ch in s:
        o = ord(ch)
        if ch == '"' or ch == '\\':
            out.append('\\' + ch)
        elif o < 0x80:
            out.append(ch)
        else:
            out.extend('\\%03o' % b for b in ch.encode('utf-8'))
    return ''.join(out)


def main():
    idx = {name: i for i, name in enumerate(IDS)}
    lines = ["/* THIS FILE IS GENERATED by tools/gen_pocket_intl.py - do not edit.",
             " * Pocket Menu strings per language, indexed by",
             " * (id - MENU_ENUM_POCKET_FIRST - 1); NULL falls back to English. */",
             "", "#define POCKET_INTL_COUNT %d" % len(IDS), ""]
    cases = []
    for suffix, lang in LANGS:
        table = [None] * len(IDS)
        merged = miyoo_strings(suffix)
        merged.update(NEW.get(suffix, {}))
        for short, val in merged.items():
            table[idx[K[short]]] = val
        if not any(table):
            continue
        name = "pocket_intl_" + suffix
        lines.append("static const char *const %s[POCKET_INTL_COUNT] = {" % name)
        for i, val in enumerate(table):
            lines.append("   %s, /* %s */" % ('"%s"' % c_escape(val) if val else "NULL", IDS[i]))
        lines.append("};")
        lines.append("")
        cases.append("      case RETRO_LANGUAGE_%s: return %s;" % (lang, name))
    lines.append("static const char *const *pocket_intl_table(unsigned lang)")
    lines.append("{")
    lines.append("   switch (lang)")
    lines.append("   {")
    lines += cases
    lines.append("      default: break;")
    lines.append("   }")
    lines.append("   return NULL;")
    lines.append("}")
    with open(os.path.join(ROOT, "menu", "pocket_menu_intl.h"), "w", encoding="ascii") as f:
        f.write("\n".join(lines) + "\n")
    print("languages: %d, ids: %d" % (len(cases), len(IDS)))


if __name__ == "__main__":
    main()
