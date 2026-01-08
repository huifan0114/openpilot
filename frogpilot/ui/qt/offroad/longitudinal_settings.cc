#include "frogpilot/ui/qt/offroad/longitudinal_settings.h"

FrogPilotLongitudinalPanel::FrogPilotLongitudinalPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  networkManager = new QNetworkAccessManager(this);

  QJsonObject shownDescriptions = QJsonDocument::fromJson(QString::fromStdString(params.get("ShownToggleDescriptions")).toUtf8()).object();
  QString className = this->metaObject()->className();

  if (!shownDescriptions.value(className).toBool(false)) {
    forceOpenDescriptions = true;
    shownDescriptions.insert(className, true);
    params.put("ShownToggleDescriptions", QJsonDocument(shownDescriptions).toJson(QJsonDocument::Compact).toStdString());
  }

  QStackedLayout *longitudinalLayout = new QStackedLayout();
  addItem(longitudinalLayout);

  FrogPilotListWidget *longitudinalList = new FrogPilotListWidget(this);

  ScrollView *longitudinalPanel = new ScrollView(longitudinalList, this);

  longitudinalLayout->addWidget(longitudinalPanel);

  FrogPilotListWidget *advancedLongitudinalTuneList = new FrogPilotListWidget(this);
  FrogPilotListWidget *aggressivePersonalityList = new FrogPilotListWidget(this);
  FrogPilotListWidget *conditionalExperimentalList = new FrogPilotListWidget(this);
  FrogPilotListWidget *curveSpeedList = new FrogPilotListWidget(this);
  FrogPilotListWidget *customDrivingPersonalityList = new FrogPilotListWidget(this);
  FrogPilotListWidget *longitudinalTuneList = new FrogPilotListWidget(this);
  FrogPilotListWidget *qolList = new FrogPilotListWidget(this);
  FrogPilotListWidget *relaxedPersonalityList = new FrogPilotListWidget(this);
  FrogPilotListWidget *speedLimitControllerList = new FrogPilotListWidget(this);
  FrogPilotListWidget *speedLimitControllerOffsetsList = new FrogPilotListWidget(this);
  FrogPilotListWidget *speedLimitControllerQOLList = new FrogPilotListWidget(this);
  FrogPilotListWidget *speedLimitControllerVisualList = new FrogPilotListWidget(this);
  FrogPilotListWidget *standardPersonalityList = new FrogPilotListWidget(this);
  FrogPilotListWidget *trafficPersonalityList = new FrogPilotListWidget(this);
  FrogPilotListWidget *weatherList = new FrogPilotListWidget(this);
  FrogPilotListWidget *weatherLowVisibilityList = new FrogPilotListWidget(this);
  FrogPilotListWidget *weatherRainList = new FrogPilotListWidget(this);
  FrogPilotListWidget *weatherRainStormList = new FrogPilotListWidget(this);
  FrogPilotListWidget *weatherSnowList = new FrogPilotListWidget(this);

  ScrollView *advancedLongitudinalTunePanel = new ScrollView(advancedLongitudinalTuneList, this);
  ScrollView *aggressivePersonalityPanel = new ScrollView(aggressivePersonalityList, this);
  ScrollView *conditionalExperimentalPanel = new ScrollView(conditionalExperimentalList, this);
  ScrollView *curveSpeedPanel = new ScrollView(curveSpeedList, this);
  ScrollView *customDrivingPersonalityPanel = new ScrollView(customDrivingPersonalityList, this);
  ScrollView *longitudinalTunePanel = new ScrollView(longitudinalTuneList, this);
  ScrollView *qolPanel = new ScrollView(qolList, this);
  ScrollView *relaxedPersonalityPanel = new ScrollView(relaxedPersonalityList, this);
  ScrollView *speedLimitControllerPanel = new ScrollView(speedLimitControllerList, this);
  ScrollView *speedLimitControllerOffsetsPanel = new ScrollView(speedLimitControllerOffsetsList, this);
  ScrollView *speedLimitControllerQOLPanel = new ScrollView(speedLimitControllerQOLList, this);
  ScrollView *speedLimitControllerVisualPanel = new ScrollView(speedLimitControllerVisualList, this);
  ScrollView *standardPersonalityPanel = new ScrollView(standardPersonalityList, this);
  ScrollView *trafficPersonalityPanel = new ScrollView(trafficPersonalityList, this);
  ScrollView *weatherLowVisibilityPanel = new ScrollView(weatherLowVisibilityList, this);
  ScrollView *weatherPanel = new ScrollView(weatherList, this);
  ScrollView *weatherRainPanel = new ScrollView(weatherRainList, this);
  ScrollView *weatherRainStormPanel = new ScrollView(weatherRainStormList, this);
  ScrollView *weatherSnowPanel = new ScrollView(weatherSnowList, this);

  longitudinalLayout->addWidget(advancedLongitudinalTunePanel);
  longitudinalLayout->addWidget(aggressivePersonalityPanel);
  longitudinalLayout->addWidget(conditionalExperimentalPanel);
  longitudinalLayout->addWidget(curveSpeedPanel);
  longitudinalLayout->addWidget(customDrivingPersonalityPanel);
  longitudinalLayout->addWidget(longitudinalTunePanel);
  longitudinalLayout->addWidget(qolPanel);
  longitudinalLayout->addWidget(relaxedPersonalityPanel);
  longitudinalLayout->addWidget(speedLimitControllerPanel);
  longitudinalLayout->addWidget(speedLimitControllerOffsetsPanel);
  longitudinalLayout->addWidget(speedLimitControllerQOLPanel);
  longitudinalLayout->addWidget(speedLimitControllerVisualPanel);
  longitudinalLayout->addWidget(standardPersonalityPanel);
  longitudinalLayout->addWidget(trafficPersonalityPanel);
  longitudinalLayout->addWidget(weatherLowVisibilityPanel);
  longitudinalLayout->addWidget(weatherPanel);
  longitudinalLayout->addWidget(weatherRainPanel);
  longitudinalLayout->addWidget(weatherRainStormPanel);
  longitudinalLayout->addWidget(weatherSnowPanel);

  const std::vector<std::tuple<QString, QString, QString, QString>> longitudinalToggles {
    {"AdvancedLongitudinalTune", tr("進階縱向調校"), tr("<b>進階的加速與煞車控制設定</b> 用於微調 openpilot 的駕駛行為。"), "../../frogpilot/assets/toggle_icons/icon_advanced_longitudinal_tune.png"},
    {"LongitudinalActuatorDelay", parent->longitudinalActuatorDelay != 0 ? QString(tr("執行器延遲（預設：%1）")).arg(QString::number(parent->longitudinalActuatorDelay, 'f', 2)) : tr("執行器延遲"), tr("<b>openpilot 發出油門或煞車命令到車輛反應之間的時間。</b> 車輛反應遲鈍請增加；若反應過度或過衝請減少。"), ""},
    {"MaxDesiredAcceleration", tr("最大加速"), tr("<b>限制 openpilot 可要求的最大加速度</b>。"), ""},
    {"StartAccel", parent->startAccel != 0 ? QString(tr("起步加速度（預設：%1）")).arg(QString::number(parent->startAccel, 'f', 2)) : tr("起步加速度"), tr("<b>從靜止起步時額外加速度。</b> 增加可更快起步；減少可讓起步更平順。"), ""},
    {"VEgoStarting", parent->vEgoStarting != 0 ? QString(tr("起步速度（預設：%1）")).arg(QString::number(parent->vEgoStarting, 'f', 2)) : tr("起步速度"), tr("<b>openpilot 判定離開停車狀態的速度。</b> 增加可減少慢爬；減少則可更快起步。"), ""},
    {"StopAccel", parent->stopAccel != 0 ? QString(tr("停止加速度（預設：%1）")).arg(QString::number(parent->stopAccel, 'f', 2)) : tr("停止加速度"), tr("<b>用於在靜止時保持車輛的煞車力。</b> 增加以防止車輛在坡道滑動；減少以獲得更平緩停止。"), ""},
    {"StoppingDecelRate", parent->stoppingDecelRate != 0 ? QString(tr("停止速率（預設：%1）")).arg(QString::number(parent->stoppingDecelRate, 'f', 2)) : tr("停止速率"), tr("<b>停止時煞車上升的速度。</b> 增加可縮短停止距離並更有力；減少則較平順。"), ""},
    {"VEgoStopping", parent->vEgoStopping != 0 ? QString(tr("停止速度（預設：%1）")).arg(QString::number(parent->vEgoStopping, 'f', 2)) : tr("停止速度"), tr("<b>openpilot 判定車輛已停下的速度。</b> 增加可較早煞車並平順停止；減少會延後但可能過頭。"), ""},

    {"ConditionalExperimental", tr("條件式實驗模式"), tr("<b>當達成設置條件時自動切換為\"實驗模式\"。</b> 使模型能在挑戰情境下做出更智慧的決策。"), "../../frogpilot/assets/toggle_icons/icon_conditional.png"},
    {"CESpeed", tr("低於"), tr("<b>當在無前車情況且速限低於此值時切換為\"實驗模式\"</b>，以協助 openpilot 更順暢處理低速情境。"), ""},
    {"CECurves", tr("前方偵測到彎道"), tr("<b>當偵測到彎道時切換為\"實驗模式\"</b>，讓模型為彎道設定適當速度。"), ""},
    {"CEStopLights", tr("\"檢測到停車燈/標誌"), tr("<b>每當駕駛模型“檢測到”紅燈或停車標誌時，切換到“實驗模式”。 </b><br><br><i><b>免責聲明</b>：openpilot 不會明確檢測交通燈或停車標誌。在“實驗模式”下，openpilot 根據攝像頭輸入做出端到端的駕駛決策，這意味著即使沒有明確的原因，它也可能會停止！</i>"), ""},
    {"CELead", tr("前方偵測到前車"), tr("<b>當偵測到較慢或靜止車輛時切換為\"實驗模式\"。</b> 在某些車輛上可使煞車更平順且更可靠。"), ""},
    {"CENavigation", tr("基於導航"), tr("<b>在使用 \"Navigate on openpilot\" (NOO) 且接近路線上的交叉路口或轉彎時切換為\"實驗模式\"</b>，讓模型為即將到來的動作設定適當速度。"), ""},
    {"CEModelStopTime", tr("預測停車時間"), tr("<b>當 openpilot 預測在設定時間內會停車時切換為\"實驗模式\"。</b> 此通常由模型偵測到紅燈或停車標誌觸發。<br><br><i><b>免責聲明</b>：openpilot 並不會明確偵測紅綠燈或停車標誌。在\"實驗模式\"中，openpilot 會根據相機輸入做端到端的駕駛決策，因此可能在沒有明確理由時停車。</i>"), ""},
    {"CESignalSpeed", tr("方向燈低於"), tr("<b>在使用方向燈且速度低於設定值時切換為\"實驗模式\"</b>，讓模型為左右轉彎選擇更平順的速度。"), ""},
    {"ShowCEMStatus", tr("狀態小工具"), tr("<b>在行車畫面顯示觸發\"實驗模式\"的條件</b>。"), ""},

    {"CurveSpeedController", tr("彎道速度控制器"), tr("<b>使用從你的駕駛習慣學到的資料自動減速以應對即將到來的彎道</b>，像你平常駕駛時一樣調整過彎速度。"), "../../frogpilot/assets/toggle_icons/icon_speed_map.png"},
    {"CalibratedLateralAcceleration", tr("校正側向加速度"), tr("<b>從收集的行車資料學到的側向加速度。</b> 此值決定 openpilot 過彎速度。數值越高允許更快過彎；數值越低則減速以獲得更平順的轉向。"), ""},
    {"CalibrationProgress", tr("校正進度"), tr("<b>已收集到多少彎道資料。</b> 這是一個進度指示；數值保持較低且很少到達 100% 是正常現象。"), ""},
    {"ResetCurveData", tr("重設彎道資料"), tr("<b>重設為 \"彎道速度控制器\" 收集的使用者資料。</b>"), ""},
    {"ShowCSCStatus", tr("狀態小工具"), tr("<b>在行車畫面顯示 \"彎道速度控制器\" 的目標速度。</b>"), ""},

    {"CustomPersonalities", tr("駕駛風格"), tr("<b>自訂 \"駕駛風格\"，以更符合你的駕駛習慣。</b>"), "../../frogpilot/assets/toggle_icons/icon_personality.png"},

    {"TrafficPersonalityProfile", tr("塞車模式"), tr("<b>自訂 \"塞車模式\" 的風格設定。</b> 適用於頻繁停走的路況。"), "../../frogpilot/assets/stock_theme/distance_icons/traffic.png"},
    {"TrafficFollow", tr("跟車距離"), tr("<b>在 \"塞車模式\" 下與前車的最小跟車距離。</b> 隨速度提升，openpilot 會在此值與 \"進取\" 檔案之間調整。增加可獲得更大空間；減少可縮短間距。"), ""},
    {"TrafficJerkAcceleration", tr("加速平順度"), tr("<b>在 \"塞車模式\" 中 openpilot 加速的平順程度。</b> 增加可獲得較溫和的起步；減少則更快但較突兀。"), ""},
    {"TrafficJerkDeceleration", tr("煞車平順度"), tr("<b>在 \"塞車模式\" 中 openpilot 煞車的平順程度。</b> 增加可獲得較溫和的停止；減少則更迅速但較激烈。"), ""},
    {"TrafficJerkDanger", tr("安全間隙偏好"), tr("<b>在 \"塞車模式\" 中 openpilot 與前車保持的額外安全距離。</b> 增加可獲得更保守的間距；減少則更接近前車。"), ""},
    {"TrafficJerkSpeedDecrease", tr("減速反應"), tr("<b>在 \"塞車模式\" 中 openpilot 減速的平順程度。</b> 增加可獲得較緩和的減速；減少則較快但較突兀。"), ""},
    {"TrafficJerkSpeed", tr("加速反應"), tr("<b>在 \"塞車模式\" 中 openpilot 加速的反應程度。</b> 增加可獲得較平順的加速；減少則較快但較突兀。"), ""},
    {"ResetTrafficPersonality", tr("重設為預設"), tr("<b>將 \"塞車模式\" 設定重設為預設值。</b>"), ""},

    {"AggressivePersonalityProfile", tr("進取"), tr("<b>自訂 \"進取\" 風格設定。</b> 適合較積極的駕駛，保持較小車距。"), "../../frogpilot/assets/stock_theme/distance_icons/aggressive.png"},
    {"AggressiveFollow", tr("跟車距離"), tr("<b>使用 \"進取\" 檔案時 openpilot 跟隨前車的秒數。</b> 增加可獲得更多空間；減少則間距更緊密。<br><br>預設：1.25 秒。"), ""},
    {"AggressiveJerkAcceleration", tr("加速平順度"), tr("<b>使用 \"進取\" 檔案時 openpilot 加速的平順程度。</b> 增加可獲得較溫和的起步；減少則更快但較突兀。"), ""},
    {"AggressiveJerkDeceleration", tr("煞車平順度"), tr("<b>使用 \"進取\" 檔案時 openpilot 煞車的平順程度。</b> 增加可獲得較溫和的停止；減少則更迅速但較激烈。"), ""},
    {"AggressiveJerkDanger", tr("安全間隙偏好"), tr("<b>使用 \"進取\" 檔案時 openpilot 與前車保持的額外空間。</b> 增加可獲得更保守的間距；減少則更接近前車。"), ""},
    {"AggressiveJerkSpeedDecrease", tr("減速反應"), tr("<b>使用 \"進取\" 檔案時 openpilot 減速的平順程度。</b> 增加可獲得較緩和的減速；減少則較快但較突兀。"), ""},
    {"AggressiveJerkSpeed", tr("加速反應"), tr("<b>使用 \"進取\" 檔案時 openpilot 加速的反應程度。</b> 增加可獲得較平順的加速；減少則較快但較突兀。"), ""},
    {"ResetAggressivePersonality", tr("重設為預設"), tr("<b>將 \"進取\" 風格重設為預設值。</b>"), ""},

    {"StandardPersonalityProfile", tr("標準"), tr("<b>自訂 \"標準\" 風格設定。</b> 適合平衡的駕駛風格與中等車距。"), "../../frogpilot/assets/stock_theme/distance_icons/standard.png"},
    {"StandardFollow", tr("跟車距離"), tr("<b>使用 \"標準\" 檔案時 openpilot 跟隨前車的秒數。</b> 增加可獲得更多空間；減少則間距更緊密。<br><br>預設：1.45 秒。"), ""},
    {"StandardJerkAcceleration", tr("加速平順度"), tr("<b>使用 \"標準\" 檔案時 openpilot 加速的平順程度。</b> 增加可獲得較溫和的起步；減少則更快但較突兀。"), ""},
    {"StandardJerkDeceleration", tr("煞車平順度"), tr("<b>使用 \"標準\" 檔案時 openpilot 煞車的平順程度。</b> 增加可獲得較溫和的停止；減少則更迅速但較激烈。"), ""},
    {"StandardJerkDanger", tr("安全間隙偏好"), tr("<b>使用 \"標準\" 檔案時 openpilot 與前車保持的額外空間。</b> 增加可獲得更保守的間距；減少則更接近前車。"), ""},
    {"StandardJerkSpeedDecrease", tr("減速反應"), tr("<b>使用 \"標準\" 檔案時 openpilot 減速的平順程度。</b> 增加可獲得較緩和的減速；減少則較快但較突兀。"), ""},
    {"StandardJerkSpeed", tr("加速反應"), tr("<b>使用 \"標準\" 檔案時 openpilot 加速的反應程度。</b> 增加可獲得較平順的加速；減少則較快但較突兀。"), ""},
    {"ResetStandardPersonality", tr("重設為預設"), tr("<b>將 \"標準\" 風格重設為預設值。</b>"), ""},

    {"RelaxedPersonalityProfile", tr("舒適"), tr("<b>自訂 \"舒適\" 風格設定。</b> 適合較平順、舒適的駕駛並保持較大車距。"), "../../frogpilot/assets/stock_theme/distance_icons/relaxed.png"},
    {"RelaxedFollow", tr("跟車距離"), tr("<b>使用 \"舒適\" 檔案時 openpilot 跟隨前車的秒數。</b> 增加可獲得更多空間；減少則間距更緊密。<br><br>預設：1.75 秒。"), ""},
    {"RelaxedJerkAcceleration", tr("加速平順度"), tr("<b>使用 \"舒適\" 檔案時 openpilot 加速的平順程度。</b> 增加可獲得較溫和的起步；減少則更快但較突兀。"), ""},
    {"RelaxedJerkDeceleration", tr("煞車平順度"), tr("<b>使用 \"舒適\" 檔案時 openpilot 煞車的平順程度。</b> 增加可獲得較溫和的停止；減少則更迅速但較激烈。"), ""},
    {"RelaxedJerkDanger", tr("安全間隙偏好"), tr("<b>使用 \"舒適\" 檔案時 openpilot 與前車保持的額外空間。</b> 增加可獲得更保守的間距；減少則更接近前車。"), ""},
    {"RelaxedJerkSpeedDecrease", tr("減速反應"), tr("<b>使用 \"舒適\" 檔案時 openpilot 減速的平順程度。</b> 增加可獲得較緩和的減速；減少則較快但較突兀。"), ""},
    {"RelaxedJerkSpeed", tr("加速反應"), tr("<b>使用 \"舒適\" 檔案時 openpilot 加速的反應程度。</b> 增加可獲得較平順的加速；減少則較快但較突兀。"), ""},
    {"ResetRelaxedPersonality", tr("重設為預設"), tr("<b>將 \"舒適\" 風格重設為預設值。</b>"), ""},

    {"LongitudinalTune", tr("縱向調校"), tr("<b>加速與煞車控制設定</b> 用於微調 openpilot 的駕駛表現。"), "../../frogpilot/assets/toggle_icons/icon_longitudinal_tune.png"},
    {"AccelerationProfile", tr("加速設定"), tr("<b>openpilot 加速的快慢設定。</b> \"節能\" 輕柔且省油，\"運動\" 較有力且反應快，\"運動+\" 為允許的最大加速率。"), ""},
    {"DecelerationProfile", tr("減速設定"), tr("<b>openpilot 減速的力度設定。</b> \"節能\" 偏向滑行，\"運動\" 則採用較強的煞車。"), ""},
    {"HumanAcceleration", tr("類人加速"), tr("<b>模仿人類駕駛的加速行為</b>，在低速時平順放油門，並在起步時給予額外動力。"), ""},
    {"HumanFollowing", tr("類人跟車"), tr("<b>模仿人類駕駛的跟車行為</b>，在較快車輛前方縮短距離以快速起步，並動態調整跟車距離以達到更平順與高效率的煞車。"), ""},
    {"HumanLaneChanges", tr("類人變道"), tr("<b>模仿人類駕駛的變道行為</b>，在變道過程中預測並追蹤鄰近車輛。"), ""},
    {"LeadDetectionThreshold", tr("前車偵測靈敏度"), tr("<b>openpilot 偵測車輛的靈敏度。</b> 靈敏度提高可以在較遠距離更早偵測，但可能對非車輛物體也有反應；降低則較保守並減少誤偵測。"), ""},
    {"TacoTune", tr("\"Taco Bell Run\" Turn Speed Hack"), tr("<b>The turn-speed hack from comma's 2022 \"Taco Bell Run\".</b> Designed to slow down for left and right turns."), ""},

    {"QOLLongitudinal", tr("便利功能"), tr("<b>各種加速與煞車的雜項調整</b>，用於微調 openpilot 的駕駛感受。"), "../../frogpilot/assets/toggle_icons/icon_quality_of_life.png"},
    {"CustomCruise", tr("巡航增減值"), tr("<b>每次按下 + 或 - 巡航按鈕時，設定速度增加或減少多少。</b>"), ""},
    {"CustomCruiseLong", tr("長按巡航增減值"), tr("<b>長按 + 或 - 巡航按鈕時，設定速度增加或減少多少。</b>"), ""},
    {"ForceStops", tr("強制於偵測到的紅綠燈/停車標誌停車"), tr("<b>當駕駛模型 \"偵測到\" 紅燈或停車標誌時強制 openpilot 停車。</b><br><br><i><b>免責聲明</b>：openpilot 並不會明確偵測紅綠燈或停車標誌。在 \"實驗模式\" 中，openpilot 會基於相機輸入做端到端的駕駛決策，因此可能在沒有明確理由時停車。</i>"), ""},
    {"IncreasedStoppedDistance", tr("增加停車時距離："), tr("<b>在停車時增加與前車的空間。</b> 增加可獲得更多空間；減少則距離較短。"), ""},
    {"MapGears", tr("將加減速對應至檔位"), tr("<b>將加速或減速檔案對應到車輛的 \"節能\" 或 \"運動\" 檔位。</b>"), ""},
    {"SetSpeedOffset", tr("設定速度偏移量："), tr("<b>將設定速度增加所選的偏移量。</b> 例如，若你通常超速 5 mph，則設定 +5。"), ""},
    {"ReverseCruise", tr("反向巡航增量"), tr("<b>反轉巡航按鈕的行為</b>，讓短按增加 5 而非 1。"), ""},
    {"WeatherPresets", tr("天氣狀況補償"), tr("<b>根據實時天氣自動調整駕駛行為。 </b>有助於在能見度低、下雨或下雪時保持舒適性和安全性。"), ""},

    {"LowVisibilityOffsets", tr("低能見度"), tr("<b>針對霧、霾或其他低能見度條件的駕駛調整。</b>"), ""},
    {"IncreaseFollowingLowVisibility", tr("將跟隨距離增加："), tr("<b>在能見度較低的情況下，在引導車輛後面添加額外的空間。 </b>增加以獲得更多空間；減少間隙。"), ""},
    {"IncreasedStoppedDistanceLowVisibility", tr("將停止距離增加："), tr("<b>在能見度低的情況下停在車輛後面時添加額外的緩衝區。 </b>增加以獲得更多空間；間隙較短時減少。"), ""},
    {"ReduceAccelerationLowVisibility", tr("減少加速度："), tr("<b>降低低能見度時的最大加速度。</b> 增加以獲得較軟的起飛；減少為更快但不太穩定的起飛。"), ""},
    {"ReduceLateralAccelerationLowVisibility", tr("在彎道上降低速度："), tr("<b>在能見度較低的情況下通過彎道時降低所需的速度。</b> 增加以獲得更安全、更平緩的轉彎；降低以獲得更激進的彎道駕駛。"), ""},

    {"RainOffsets", tr("Rain"), tr("<b>雨天行駛調整。</b>"), ""},
    {"IncreaseFollowingRain", tr("增加跟車距離："), tr("<b>在雨中在領先車輛後面添加額外的空間。</b> 增加以獲得更多空間；減少以獲得更小的間隙。"), ""},
    {"IncreasedStoppedDistanceRain", tr("將停止距離增加："), tr("<b>在雨中停在車輛後面時添加額外的緩衝區。</b> 增加以獲得更多空間；減少以獲得更短的間隙。"), ""},
    {"ReduceAccelerationRain", tr("減少加速度："), tr("<b>降低雨中的最大加速度。</b> 增加以實現較軟的起飛；減少以實現更快但不太穩定的起飛。"), ""},
    {"ReduceLateralAccelerationRain", tr("在彎道上降低速度："), tr("<b>在雨中行駛通過彎道時降低所需速度。</b> 增加以獲得更安全、更平緩的轉彎；降低以獲得更激進的彎道駕駛。"), ""},

    {"RainStormOffsets", tr("暴雨"), tr("<b>針對暴雨的駕駛調整。</b>"), ""},
    {"IncreaseFollowingRainStorm", tr("將跟車距離增加："), tr("<b>在暴雨中在領先車輛後面添加額外的空間。</b> 增加以獲得更多空間；減少以獲得更小的間隙。"), ""},
    {"IncreasedStoppedDistanceRainStorm", tr("將停止距離增加："), tr("<b>在暴雨中停在車輛後面時添加額外的緩衝區。</b>增加以獲得更多空間；減少以獲得更短的間隙。"), ""},
    {"ReduceAccelerationRainStorm", tr("減少加速度："), tr("<b>降低暴雨中的最大加速度。</b> 增加以實現較軟的起飛；減少以實現更快但不太穩定的起飛。"), ""},
    {"ReduceLateralAccelerationRainStorm", tr("在彎道上降低速度："), tr("<b>在暴雨中行駛通過彎道時降低所需的速度。</b> 增加以獲得更安全、更平緩的轉彎；減少以獲得更激進的彎道駕駛。"), ""},

    {"SnowOffsets", tr("Snow"), tr("<b>針對下雪條件的駕駛調整。</b>"), ""},
    {"IncreaseFollowingSnow", tr("將跟車距離增加："), tr("<b>在雪中在引導車輛後面添加額外的空間。</b> 增加以獲得更多空間；減少以獲得更小的間隙。"), ""},
    {"IncreasedStoppedDistanceSnow", tr("將停止距離增加："), tr("<b>在雪中停在車輛後面時添加額外的緩衝區。</b>增加以獲得更多空間；減少以獲得更短的間隙。"), ""},
    {"ReduceAccelerationSnow", tr("減少加速度："), tr("<b>降低雪地最大加速度。</b> 增加以實現較軟的起飛；減少以實現更快但不太穩定的起飛。"), ""},
    {"ReduceLateralAccelerationSnow", tr("在彎道上降低速度："), tr("<b>在雪地中通過彎道時降低所需速度。</b> 增加以獲得更安全、更平緩的轉彎；減少以獲得更激進的彎道駕駛。"), ""},

    {"SetWeatherKey", tr("設置您自己的密鑰"), tr("<b>設置您自己的\"OpenWeatherMap\"密鑰以提高天氣更新率。</b><br><br><i>個人密鑰每天可免費調用 1​​,000 次，每分鐘更新一次。默認密鑰是共享的，每 15 分鐘更新一次。</i>"), ""},

    {"SpeedLimitController", tr("速度限制控制器"), tr("<b>將 openpilot 的最大行駛速度限制為當前速度限制</b>，該速度限制是從下載的地圖、Mapbox、Navigate on openpilot 或受支持車輛（福特、Genesis、現代、起亞、雷克薩斯、豐田）的儀表板獲取的。"), " ../../frogpilot/assets/toggle_icons/icon_speed_limit.png"},

    {"SLCFallback", tr("備用速限"), tr("<b>未發現限速時“限速控制器”使用的速度。</b><br><br>-<b>設置速度</b>：使用巡航設定速度<br>-<b>實驗模式</b>：使用駕駛模型估計限制<br>-<b>先前限制</b>：繼續使用最後確認的限制"), ""},

    {"SLCOverride", tr("覆蓋速度"), tr("<b>您手動駕駛速度超過公佈的限制後，\"速度限制控制器\"使用的速度。</b><br><br>-<b>使用油門踏板設置</b>：使用踩下油門時達到的最高速度<br>-<b>最大設置速度</b>：使用巡航設置速度<br><br>當 openpilot 脫離時覆蓋清除。"), ""},
    {"SLCQOL", tr("生活質量"), tr("<b>其他“速度限制控制器”更改</b>以微調 openpilot 的駕駛方式。"), ""},
    {"SLCConfirmation", tr("確認新的速度限制"), tr("<b>更改為新的速度限制之前先詢問。</b>要接受，請點擊屏幕上閃爍的小部件或按巡航增加按鈕。要拒絕，請按巡航減少按鈕或忽略提示 30 秒。"), ""},
    {"ForceMPHDashboard", tr("從儀表板強制 MPH"), tr("<b>始終讀取以英里為單位的儀表板速度限制標誌。</b>如果集群顯示英里/小時但限制被解釋為公里/小時，請打開此功能。"), ""},
    {"SLCLookaheadHigher", tr("更高限制前瞻時間"), tr("<b>根據下載的地圖數據，openpilot 預計即將到來的更高速度限制有多遠</b>。"), ""},
    {"SLCLookaheadLower", tr("前瞻時間下限"), tr("<b>openpilot 根據下載的地圖數據預測即將到來的較低速度限制多遠</b>。"), ""},
    {"SetSpeedLimit", tr("匹配接合速度限制"), tr("<b>首次啟用 openpilot 時，自動將最大速度設置為當前發布的限制。</b>"), ""},
    {"SLCMapboxFiller", tr("使用 Mapbox 作為後備"), tr("<b>當沒有其他來源可用時使用 Mapbox 限速數據。</b>"), ""},
    {"SLCPriority", tr("速度限制源優先級"), tr("<b>當多個速度限制源可用時的速度限制源順序</b>。"), ""},
    {"SLCOffsets", tr("速度限制偏移"), tr("<b>為發布的速度限制添加偏移</b>，以更好地匹配您的駕駛風格。"), ""},
    {"Offset1", tr("速度偏移 (0–24 英里/小時)"), tr("<b>在 0 到 24 英里/小時之間偏移張貼的速度限制多少</b>。"), ""},
    {"Offset2", tr("速度偏移 (25–34 英里/小時)"), tr("<b>在 25 到 34 英里/小時之間偏移張貼的速度限制多少</b>。"), ""},
    {"Offset3", tr("速度偏移 (35–44 英里/小時)"), tr("<b>在 35 到 44 英里/小時之間偏移張貼的速度限制多少</b>。"), ""},
    {"Offset4", tr("速度偏移 (45–54 英里/小時)"), tr("<b>在 45 到 54 英里/小時之間偏移張貼的速度限制多少</b>。"), ""},
    {"Offset5", tr("速度偏移 (55–64 英里/小時)"), tr("<b>在 55 到 64 英里/小時之間偏移張貼的速度限制多少</b>。"), ""},
    {"Offset6", tr("速度偏移 (65–74 英里/小時)"), tr("<b>在 65 到 74 英里/小時之間偏移張貼的速度限制多少</b>。"), ""},
    {"Offset7", tr("速度偏移 (75–99 英里/小時)"), tr("<b>在 75 到 99 英里/小時之間偏移張貼的速度限制多少</b>。"), ""},
    {"SLCVisuals", tr("視覺設置"), tr("<b>視覺\"速度限制控制器\"更改</b>以微調駕駛屏幕的外觀。"), ""},
    {"ShowSLCOffset", tr("顯示速度限制偏移量"), tr("<b>在駕駛屏幕上顯示與發布限制的當前偏移量</b>。"), ""},
    {"SpeedLimitSources", tr("顯示限速來源"), tr("<b>在駕駛屏幕上顯示限速源及其當前值</b>。"), ""}
  };

  for (const auto &[param, title, desc, icon] : longitudinalToggles) {
    AbstractControl *longitudinalToggle;

    if (param == "AdvancedLongitudinalTune") {
      FrogPilotManageControl *advancedLongitudinalTuneToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(advancedLongitudinalTuneToggle, &FrogPilotManageControl::manageButtonClicked, [longitudinalLayout, advancedLongitudinalTunePanel]() {
        longitudinalLayout->setCurrentWidget(advancedLongitudinalTunePanel);
      });
      longitudinalToggle = advancedLongitudinalTuneToggle;
    } else if (param == "LongitudinalActuatorDelay") {
      longitudinalActuatorDelayToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 1, tr(" 秒"), std::map<float, QString>(), 0.01);
      longitudinalToggle = longitudinalActuatorDelayToggle;
    } else if (param == "MaxDesiredAcceleration") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0.1, 4.0, tr(" 米/秒²"), std::map<float, QString>(), 0.1);
    } else if (param == "StartAccel") {
      startAccelToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 4, tr(" 米/秒²"), std::map<float, QString>(), 0.01, true);
      longitudinalToggle = startAccelToggle;
    } else if (param == "VEgoStarting") {
      vEgoStartingToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0.01, 1, tr(" 米/秒²"), std::map<float, QString>(), 0.01);
      longitudinalToggle = vEgoStartingToggle;
    } else if (param == "StopAccel") {
      stopAccelToggle = new FrogPilotParamValueControl(param, title, desc, icon, -4, 0, tr(" 米/秒²"), std::map<float, QString>(), 0.01, true);
      longitudinalToggle = stopAccelToggle;
    } else if (param == "StoppingDecelRate") {
      stoppingDecelRateToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0.001, 1, tr(" 米/秒²"), std::map<float, QString>(), 0.001, true);
      longitudinalToggle = stoppingDecelRateToggle;
    } else if (param == "VEgoStopping") {
      vEgoStoppingToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0.01, 1, tr(" 米/秒²"), std::map<float, QString>(), 0.01);
      longitudinalToggle = vEgoStoppingToggle;

    } else if (param == "ConditionalExperimental") {
      FrogPilotManageControl *conditionalExperimentalToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(conditionalExperimentalToggle, &FrogPilotManageControl::manageButtonClicked, [longitudinalLayout, conditionalExperimentalPanel]() {
        longitudinalLayout->setCurrentWidget(conditionalExperimentalPanel);
      });
      longitudinalToggle = conditionalExperimentalToggle;
    } else if (param == "CESpeed") {
      FrogPilotParamValueControl *CESpeed = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, tr(" 英里/小時"), std::map<float, QString>(), 1, true, 175);
      FrogPilotParamValueControl *CESpeedLead = new FrogPilotParamValueControl("CESpeedLead", tr("有前車"), tr("<b>當在有前車情況且速度低於此值時切換為\"實驗模式\"</b>，以協助 openpilot 更順暢處理低速情境。"), icon, 0, 99, tr(" 英里/小時"), std::map<float, QString>(), 1, true, 175);
      FrogPilotDualParamValueControl *conditionalSpeeds = new FrogPilotDualParamValueControl(CESpeed, CESpeedLead);
      longitudinalToggle = reinterpret_cast<AbstractControl*>(conditionalSpeeds);
    } else if (param == "CECurves") {
      std::vector<QString> curveToggles{"CECurvesLead"};
      std::vector<QString> curveToggleNames{tr("有前車")};
      longitudinalToggle = new FrogPilotButtonToggleControl(param, title, desc, icon, curveToggles, curveToggleNames);
    } else if (param == "CELead") {
      std::vector<QString> leadToggles{"CESlowerLead", "CEStoppedLead"};
      std::vector<QString> leadToggleNames{tr("較慢前車"), tr("靜止前車")};
      longitudinalToggle = new FrogPilotButtonToggleControl(param, title, desc, icon, leadToggles, leadToggleNames);
    } else if (param == "CENavigation") {
      std::vector<QString> navigationToggles{"CENavigationIntersections", "CENavigationTurns", "CENavigationLead"};
      std::vector<QString> navigationToggleNames{tr("交叉路口"), tr("轉彎"), tr("有前車")};
      longitudinalToggle = new FrogPilotButtonToggleControl(param, title, desc, icon, navigationToggles, navigationToggleNames);
    } else if (param == "CEModelStopTime") {
      std::map<float, QString> stopTimeLabels;
      for (int i = 0; i <= 10; ++i) {
        stopTimeLabels[i] = i == 0 ? tr("關閉") : i == 1 ? QString::number(i) + tr(" 秒") : QString::number(i) + tr(" 秒");
      }
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 9, QString(), stopTimeLabels);
    } else if (param == "CESignalSpeed") {
      std::vector<QString> ceSignalToggles{"CESignalLaneDetection"};
      std::vector<QString> ceSignalToggleNames{tr("未偵測到車道時禁用")};
      longitudinalToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, 0, 99, tr(" 英里/小時"), std::map<float, QString>(), 1.0, true, ceSignalToggles, ceSignalToggleNames, true);

    } else if (param == "CurveSpeedController") {
      FrogPilotManageControl *curveControlToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(curveControlToggle, &FrogPilotManageControl::manageButtonClicked, [longitudinalLayout, curveSpeedPanel]() {
        longitudinalLayout->setCurrentWidget(curveSpeedPanel);
      });
      longitudinalToggle = curveControlToggle;
    } else if (param == "CalibrationProgress") {
      calibrationProgressLabel = new LabelControl(title, QString::number(params.getFloat("CalibrationProgress"), 'f', 2) + "%", desc);
      longitudinalToggle = calibrationProgressLabel;
    } else if (param == "CalibratedLateralAcceleration") {
      calibratedLateralAccelerationLabel = new LabelControl(title, QString::number(params.getFloat("CalibratedLateralAcceleration"), 'f', 2) + tr(" 米/秒²"), desc);
      longitudinalToggle = calibratedLateralAccelerationLabel;
    } else if (param == "ResetCurveData") {
      ButtonControl *resetCurveDataButton = new ButtonControl(title, tr("重設"), desc);
      QObject::connect(resetCurveDataButton, &ButtonControl::clicked, [this]() {
        if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重設您的曲率資料嗎？"), this)) {
          params.putFloat("CalibratedLateralAcceleration", 2.00);
          params.remove("CalibrationProgress");
          params.remove("CurvatureData");

          params_cache.putFloat("CalibratedLateralAcceleration", 2.00);
          params_cache.remove("CalibrationProgress");
          params_cache.remove("CurvatureData");

          calibratedLateralAccelerationLabel->setText(QString::number(2.00, 'f', 2) + tr(" 米/秒²"));
          calibrationProgressLabel->setText(QString::number(0.00, 'f', 2) + "%");
        }
      });
      longitudinalToggle = resetCurveDataButton;

    } else if (param == "CustomPersonalities") {
      FrogPilotManageControl *customPersonalitiesToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(customPersonalitiesToggle, &FrogPilotManageControl::manageButtonClicked, [longitudinalLayout, customDrivingPersonalityPanel]() {
        longitudinalLayout->setCurrentWidget(customDrivingPersonalityPanel);
      });
      longitudinalToggle = customPersonalitiesToggle;
    } else if (param == "ResetTrafficPersonality" || param == "ResetAggressivePersonality" || param == "ResetStandardPersonality" || param == "ResetRelaxedPersonality") {
      ButtonControl *resetButton = new ButtonControl(title, tr("重設"), desc);
      longitudinalToggle = resetButton;
    } else if (param == "TrafficPersonalityProfile") {
      FrogPilotManageControl *trafficPersonalityToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(trafficPersonalityToggle, &FrogPilotManageControl::manageButtonClicked, [longitudinalLayout, trafficPersonalityPanel, this]() {
        openSubSubPanel();

        longitudinalLayout->setCurrentWidget(trafficPersonalityPanel);

        customPersonalityOpen = true;
      });
      longitudinalToggle = trafficPersonalityToggle;
    } else if (param == "AggressivePersonalityProfile") {
      FrogPilotManageControl *aggressivePersonalityToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(aggressivePersonalityToggle, &FrogPilotManageControl::manageButtonClicked, [longitudinalLayout, aggressivePersonalityPanel, this]() {
        openSubSubPanel();

        longitudinalLayout->setCurrentWidget(aggressivePersonalityPanel);

        customPersonalityOpen = true;
      });
      longitudinalToggle = aggressivePersonalityToggle;
    } else if (param == "StandardPersonalityProfile") {
      FrogPilotManageControl *standardPersonalityToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(standardPersonalityToggle, &FrogPilotManageControl::manageButtonClicked, [longitudinalLayout, standardPersonalityPanel, this]() {
        openSubSubPanel();

        longitudinalLayout->setCurrentWidget(standardPersonalityPanel);

        customPersonalityOpen = true;
      });
      longitudinalToggle = standardPersonalityToggle;
    } else if (param == "RelaxedPersonalityProfile") {
      FrogPilotManageControl *relaxedPersonalityToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(relaxedPersonalityToggle, &FrogPilotManageControl::manageButtonClicked, [longitudinalLayout, relaxedPersonalityPanel, this]() {
        openSubSubPanel();

        longitudinalLayout->setCurrentWidget(relaxedPersonalityPanel);

        customPersonalityOpen = true;
      });
      longitudinalToggle = relaxedPersonalityToggle;
    } else if (aggressivePersonalityKeys.contains(param) || standardPersonalityKeys.contains(param) || relaxedPersonalityKeys.contains(param) || trafficPersonalityKeys.contains(param)) {
      if (param == "TrafficFollow" || param == "AggressiveFollow" || param == "StandardFollow" || param == "RelaxedFollow") {
        std::map<float, QString> followTimeLabels;
        for (float i = 0; i <= 3; i += 0.01) {
          followTimeLabels[i] = std::lround(i / 0.01) == 1 / 0.01 ? QString::number(i, 'f', 2) + tr(" 秒") : QString::number(i, 'f', 2) + tr(" 秒");
        }
        if (param == "TrafficFollow") {
          longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0.5, 3, QString(), followTimeLabels, 0.01, true);
        } else {
          longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 3, QString(), followTimeLabels, 0.01, true);
        }
      } else {
        longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 25, 200, "%");
      }

    } else if (param == "LongitudinalTune") {
      FrogPilotManageControl *longitudinalTuneToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(longitudinalTuneToggle, &FrogPilotManageControl::manageButtonClicked, [longitudinalLayout, longitudinalTunePanel]() {
        longitudinalLayout->setCurrentWidget(longitudinalTunePanel);
      });
      longitudinalToggle = longitudinalTuneToggle;
    } else if (param == "AccelerationProfile") {
      std::vector<QString> accelerationProfiles{tr("標準"), tr("節能"), tr("運動"), tr("運動+")};
      ButtonParamControl *accelerationProfileToggle = new ButtonParamControl(param, title, desc, icon, accelerationProfiles);
      longitudinalToggle = accelerationProfileToggle;
    } else if (param == "DecelerationProfile") {
      std::vector<QString> decelerationProfiles{tr("標準"), tr("節能"), tr("運動")};
      ButtonParamControl *decelerationProfileToggle = new ButtonParamControl(param, title, desc, icon, decelerationProfiles);
      longitudinalToggle = decelerationProfileToggle;
    } else if (param == "LeadDetectionThreshold") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 25, 50, "%");

    } else if (param == "QOLLongitudinal") {
      FrogPilotManageControl *qolLongitudinalToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(qolLongitudinalToggle, &FrogPilotManageControl::manageButtonClicked, [longitudinalLayout, qolPanel]() {
        longitudinalLayout->setCurrentWidget(qolPanel);
      });
      longitudinalToggle = qolLongitudinalToggle;
    } else if (param == "CustomCruise") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 99, tr(" 英里/小時"));
    } else if (param == "CustomCruiseLong") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 99, tr(" 英里/小時"));
    } else if (param == "IncreasedStoppedDistance") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 10, tr(" 英尺"));
    } else if (param == "MapGears") {
      std::vector<QString> mapGearsToggles{"MapAcceleration", "MapDeceleration"};
      std::vector<QString> mapGearsToggleNames{tr("加速"), tr("減速")};
      longitudinalToggle = new FrogPilotButtonToggleControl(param, title, desc, icon, mapGearsToggles, mapGearsToggleNames);
    } else if (param == "SetSpeedOffset") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, tr(" 英里/小時"));
    } else if (param == "WeatherPresets") {
      FrogPilotManageControl *weatherToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(weatherToggle, &FrogPilotManageControl::manageButtonClicked, [longitudinalLayout, weatherPanel, this]() {
        openSubSubPanel();

        longitudinalLayout->setCurrentWidget(weatherPanel);

        qolOpen = true;
      });
      longitudinalToggle = weatherToggle;
    } else if (param == "SetWeatherKey") {
      weatherKeyControl = new FrogPilotButtonsControl(title, desc, icon, {tr("添加"), tr("測試")});
      QObject::connect(weatherKeyControl, &FrogPilotButtonsControl::buttonClicked, [this](int id) {
        if (id == 0) {
          if (!params.get("WeatherToken").empty()) {
            if (FrogPilotConfirmationDialog::yesorno(tr("您確定要刪除您的密鑰嗎？"), this)) {
              params.remove("WeatherToken");
              params_cache.remove("WeatherToken");

              weatherKeyControl->setText(0, tr("添加"));
              weatherKeyControl->setVisibleButton(1, false);
            }
          } else {
            int keyLength = 32;
            QString currentKey = QString::fromStdString(params.get("WeatherToken"));
            QString newKey = InputDialog::getText(tr("輸入您的“OpenWeatherMap”密鑰"), this, tr("Characters: 0/%1").arg(keyLength), false, -1, currentKey, keyLength).trimmed();
            if (!newKey.isEmpty()) {
              params.put("WeatherToken", newKey.toStdString());

              weatherKeyControl->setText(0, tr("消除"));
              weatherKeyControl->setVisibleButton(1, true);
            }
          }
        } else if (id == 1) {
          weatherKeyControl->setValue(tr("測試中..."));

          QString key = QString::fromStdString(params.get("WeatherToken")).trimmed();
          QString url30 = QString("https://api.openweathermap.org/data/3.0/onecall?lat=42.4293&lon=-83.9850&exclude=current,minutely,hourly,daily,alerts&appid=%1").arg(key);

          QNetworkRequest request(url30);
          QNetworkReply *reply = networkManager->get(request);
          QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
            reply->deleteLater();

            if (reply->error() == QNetworkReply::NoError) {
              weatherKeyControl->setValue("");
              ConfirmationDialog::alert(tr("密鑰有效!"), this);
              return;
            }

            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == 401 || status == 403) {
              QString url25 = QString("https://api.openweathermap.org/data/2.5/weather?lat=42.4293&lon=-83.9850&appid=%1").arg(key);

              QNetworkRequest request25(url25);
              QNetworkReply *reply25 = networkManager->get(request25);
              QObject::connect(reply25, &QNetworkReply::finished, this, [=]() {
                reply25->deleteLater();

                weatherKeyControl->setValue("");
                if (reply25->error() == QNetworkReply::NoError) {
                  ConfirmationDialog::alert(tr("您的密鑰對於 2.5 版本有效，但強烈建議使用 3.0 版本！請訂閱“One Call API 3.0”計劃！"), this);
                } else {
                   int status25 = reply25->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                   ConfirmationDialog::alert(tr("無效密鑰！ (Error: %1)").arg(status25), this);
                }
              });
            } else {
              weatherKeyControl->setValue("");
              ConfirmationDialog::alert(tr("發生錯誤: %1").arg(reply->errorString()), this);
            }
          });
        }
      });
      longitudinalToggle = weatherKeyControl;
    } else if (param == "LowVisibilityOffsets") {
      ButtonControl *manageLowVisibilitOffsetsButton = new ButtonControl(title, tr("管理"), desc);
      QObject::connect(manageLowVisibilitOffsetsButton, &ButtonControl::clicked, [longitudinalLayout, weatherLowVisibilityPanel, this]() {
        openSubSubSubPanel();

        longitudinalLayout->setCurrentWidget(weatherLowVisibilityPanel);

        weatherOpen = true;
      });
      longitudinalToggle = manageLowVisibilitOffsetsButton;
    } else if (param == "RainOffsets") {
      ButtonControl *manageRainOffsetsButton = new ButtonControl(title, tr("管理"), desc);
      QObject::connect(manageRainOffsetsButton, &ButtonControl::clicked, [longitudinalLayout, weatherRainPanel, this]() {
        openSubSubSubPanel();

        longitudinalLayout->setCurrentWidget(weatherRainPanel);

        weatherOpen = true;
      });
      longitudinalToggle = manageRainOffsetsButton;
    } else if (param == "RainStormOffsets") {
      ButtonControl *manageRainStormOffsetsButton = new ButtonControl(title, tr("管理"), desc);
      QObject::connect(manageRainStormOffsetsButton, &ButtonControl::clicked, [longitudinalLayout, weatherRainStormPanel, this]() {
        openSubSubSubPanel();

        longitudinalLayout->setCurrentWidget(weatherRainStormPanel);

        weatherOpen = true;
      });
      longitudinalToggle = manageRainStormOffsetsButton;
    } else if (param == "SnowOffsets") {
      ButtonControl *manageSnowOffsetsButton = new ButtonControl(title, tr("管理"), desc);
      QObject::connect(manageSnowOffsetsButton, &ButtonControl::clicked, [longitudinalLayout, weatherSnowPanel, this]() {
        openSubSubSubPanel();

        longitudinalLayout->setCurrentWidget(weatherSnowPanel);

        weatherOpen = true;
      });
      longitudinalToggle = manageSnowOffsetsButton;
    } else if (param == "IncreaseFollowingLowVisibility" || param == "IncreaseFollowingRain" || param == "IncreaseFollowingRainStorm" || param == "IncreaseFollowingSnow") {
      std::map<float, QString> followTimeLabels;
      for (float i = 0; i <= 3; i += 0.01) {
        followTimeLabels[i] = std::lround(i / 0.01) == 1 / 0.01 ? QString::number(i, 'f', 2) + tr(" 秒") : QString::number(i, 'f', 2) + tr(" 秒");
      }
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 3, QString(), followTimeLabels, 0.01, true);
    } else if (param == "IncreasedStoppedDistanceLowVisibility" || param == "IncreasedStoppedDistanceRain" || param == "IncreasedStoppedDistanceRainStorm" || param == "IncreasedStoppedDistanceSnow") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 10, tr(" feet"));
    } else if (param == "ReduceAccelerationLowVisibility" || param == "ReduceAccelerationRain" || param == "ReduceAccelerationRainStorm" || param == "ReduceAccelerationSnow") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, "%", std::map<float, QString>(), 1);
    } else if (param == "ReduceLateralAccelerationLowVisibility" || param == "ReduceLateralAccelerationRain" || param == "ReduceLateralAccelerationRainStorm" || param == "ReduceLateralAccelerationSnow") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, "%", std::map<float, QString>(), 1);

    } else if (param == "SpeedLimitController") {
      FrogPilotManageControl *speedLimitControllerToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(speedLimitControllerToggle, &FrogPilotManageControl::manageButtonClicked, [longitudinalLayout, speedLimitControllerPanel]() {
        longitudinalLayout->setCurrentWidget(speedLimitControllerPanel);
      });
      longitudinalToggle = speedLimitControllerToggle;
    } else if (param == "SLCFallback") {
      std::vector<QString> fallbackOptions{tr("Set Speed"), tr("Experimental Mode"), tr("Previous Limit")};
      ButtonParamControl *fallbackSelection = new ButtonParamControl(param, title, desc, icon, fallbackOptions);
      longitudinalToggle = fallbackSelection;
    } else if (param == "SLCOverride") {
      std::vector<QString> overrideOptions{tr("None"), tr("Set With Gas Pedal"), tr("Max Set Speed")};
      ButtonParamControl *overrideSelection = new ButtonParamControl(param, title, desc, icon, overrideOptions);
      longitudinalToggle = overrideSelection;
    } else if (param == "SLCPriority") {
      ButtonControl *slcPriorityButton = new ButtonControl(title, tr("SELECT"), desc);
      QStringList primaryPriorities = {tr("Dashboard"), tr("Map Data"), tr("Navigation"), tr("Highest"), tr("Lowest")};
      QStringList otherPriorities = {tr("None"), tr("Dashboard"), tr("Map Data"), tr("Navigation")};
      QStringList priorityPrompts = {tr("Select your primary priority"), tr("Select your secondary priority"), tr("Select your tertiary priority")};

      QObject::connect(slcPriorityButton, &ButtonControl::clicked, [=]() {
        QStringList selectedPriorities;

        for (int i = 1; i <= 3; ++i) {
          QStringList availablePriorities = i == 1 ? primaryPriorities : otherPriorities;
          availablePriorities = availablePriorities.toSet().subtract(selectedPriorities.toSet()).toList();

          if (!parent->hasDashSpeedLimits) {
            availablePriorities.removeAll(tr("Dashboard"));
          }
          if (availablePriorities.size() == 1 && availablePriorities.contains(tr("None"))) {
            break;
          }

          QString selection = MultiOptionDialog::getSelection(priorityPrompts[i - 1], availablePriorities, "", this);
          if (selection.isEmpty()) {
            break;
          }

          selectedPriorities.append(selection);

          params.put(QString("SLCPriority%1").arg(i).toStdString(), selection.toStdString());
          if (selection == tr("None")) {
            for (int j = i + 1; j <= 3; ++j) {
              params.put(QString("SLCPriority%1").arg(j).toStdString(), tr("None").toStdString());
            }
            break;
          }

          if (selection == tr("Lowest") || selection == tr("Highest")) {
            break;
          }
        }

        selectedPriorities.removeAll(tr("None"));
        if (!selectedPriorities.isEmpty()) {
          slcPriorityButton->setValue(selectedPriorities.join(", "));
        }
      });

      QStringList selectedPriorities;
      for (int i = 1; i <= 3; ++i) {
        QString priority = QString::fromStdString(params.get(QString("SLCPriority%1").arg(i).toStdString()));
        if (primaryPriorities.contains(priority)) {
          selectedPriorities.append(priority);
        }
      }
      slcPriorityButton->setValue(selectedPriorities.join(", "));

      longitudinalToggle = slcPriorityButton;
    } else if (param == "SLCOffsets") {
      ButtonControl *manageSLCOffsetsButton = new ButtonControl(title, tr( "管理設定"), desc);
      QObject::connect(manageSLCOffsetsButton, &ButtonControl::clicked, [longitudinalLayout, speedLimitControllerOffsetsPanel, this]() {
        openSubSubPanel();

        longitudinalLayout->setCurrentWidget(speedLimitControllerOffsetsPanel);

        slcOpen = true;
      });
      longitudinalToggle = manageSLCOffsetsButton;
    } else if (speedLimitControllerOffsetsKeys.contains(param)) {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, -99, 99, tr(" mph"));
    } else if (param == "SLCQOL") {
      ButtonControl *manageSLCQOLButton = new ButtonControl(title, tr( "管理設定"), desc);
      QObject::connect(manageSLCQOLButton, &ButtonControl::clicked, [longitudinalLayout, speedLimitControllerQOLPanel, this]() {
        openSubSubPanel();

        longitudinalLayout->setCurrentWidget(speedLimitControllerQOLPanel);

        slcOpen = true;
      });
      longitudinalToggle = manageSLCQOLButton;
    } else if (param == "SLCConfirmation") {
      std::vector<QString> confirmationToggles{"SLCConfirmationLower", "SLCConfirmationHigher"};
      std::vector<QString> confirmationToggleNames{tr("降低速限"), tr("提高速限")};
      longitudinalToggle = new FrogPilotButtonToggleControl(param, title, desc, icon, confirmationToggles, confirmationToggleNames);
    } else if (param == "SLCLookaheadHigher" || param == "SLCLookaheadLower") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 30, tr(" 秒"));
    } else if (param == "SLCVisuals") {
      ButtonControl *manageSLCVisualsButton = new ButtonControl(title, tr( "管理設定"), desc);
      QObject::connect(manageSLCVisualsButton, &ButtonControl::clicked, [longitudinalLayout, speedLimitControllerVisualPanel, this]() {
        openSubSubPanel();

        longitudinalLayout->setCurrentWidget(speedLimitControllerVisualPanel);

        slcOpen = true;
      });
      longitudinalToggle = manageSLCVisualsButton;

    } else {
      longitudinalToggle = new ParamControl(param, title, desc, icon);
    }

    toggles[param] = longitudinalToggle;

    if (advancedLongitudinalTuneKeys.contains(param)) {
      advancedLongitudinalTuneList->addItem(longitudinalToggle);
    } else if (aggressivePersonalityKeys.contains(param)) {
      aggressivePersonalityList->addItem(longitudinalToggle);
    } else if (conditionalExperimentalKeys.contains(param)) {
      conditionalExperimentalList->addItem(longitudinalToggle);
    } else if (curveSpeedKeys.contains(param)) {
      curveSpeedList->addItem(longitudinalToggle);
    } else if (customDrivingPersonalityKeys.contains(param)) {
      customDrivingPersonalityList->addItem(longitudinalToggle);
    } else if (longitudinalTuneKeys.contains(param)) {
      longitudinalTuneList->addItem(longitudinalToggle);
    } else if (qolKeys.contains(param)) {
      qolList->addItem(longitudinalToggle);
    } else if (relaxedPersonalityKeys.contains(param)) {
      relaxedPersonalityList->addItem(longitudinalToggle);
    } else if (speedLimitControllerKeys.contains(param)) {
      speedLimitControllerList->addItem(longitudinalToggle);
    } else if (speedLimitControllerOffsetsKeys.contains(param)) {
      speedLimitControllerOffsetsList->addItem(longitudinalToggle);
    } else if (speedLimitControllerQOLKeys.contains(param)) {
      speedLimitControllerQOLList->addItem(longitudinalToggle);
    } else if (speedLimitControllerVisualKeys.contains(param)) {
      speedLimitControllerVisualList->addItem(longitudinalToggle);
    } else if (standardPersonalityKeys.contains(param)) {
      standardPersonalityList->addItem(longitudinalToggle);
    } else if (trafficPersonalityKeys.contains(param)) {
      trafficPersonalityList->addItem(longitudinalToggle);
    } else if (weatherKeys.contains(param)) {
      weatherList->addItem(longitudinalToggle);
    } else if (weatherLowVisibilityKeys.contains(param)) {
      weatherLowVisibilityList->addItem(longitudinalToggle);
    } else if (weatherRainKeys.contains(param)) {
      weatherRainList->addItem(longitudinalToggle);
    } else if (weatherRainStormKeys.contains(param)) {
      weatherRainStormList->addItem(longitudinalToggle);
    } else if (weatherSnowKeys.contains(param)) {
      weatherSnowList->addItem(longitudinalToggle);
    } else {
      longitudinalList->addItem(longitudinalToggle);

      parentKeys.insert(param);
    }

    if (FrogPilotManageControl *frogPilotManageToggle = qobject_cast<FrogPilotManageControl*>(longitudinalToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotManageControl::manageButtonClicked, [this]() {
        emit openSubPanel();
        openDescriptions(forceOpenDescriptions, toggles);
      });
    }

    QObject::connect(longitudinalToggle, &AbstractControl::hideDescriptionEvent, [this]() {
      update();
    });
    QObject::connect(longitudinalToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  QSet<QString> forceUpdateKeys = {"HumanAcceleration", "LongitudinalTune"};
  for (const QString &key : forceUpdateKeys) {
    QObject::connect(static_cast<ToggleControl*>(toggles[key]), &ToggleControl::toggleFlipped, this, &FrogPilotLongitudinalPanel::updateToggles);
  }

  FrogPilotParamValueControl *trafficFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficFollow"]);
  FrogPilotParamValueControl *trafficAccelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkAcceleration"]);
  FrogPilotParamValueControl *trafficDecelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkDeceleration"]);
  FrogPilotParamValueControl *trafficDangerToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkDanger"]);
  FrogPilotParamValueControl *trafficSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkSpeed"]);
  FrogPilotParamValueControl *trafficSpeedDecreaseToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkSpeedDecrease"]);
  FrogPilotButtonsControl *trafficResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetTrafficPersonality"]);
  QObject::connect(trafficResetButton, &FrogPilotButtonsControl::buttonClicked, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重置 <b>塞車模式</b>嗎?"), this)) {
      params.putFloat("TrafficFollow", params_default.getFloat("TrafficFollow"));
      params.putFloat("TrafficJerkAcceleration", params_default.getFloat("TrafficJerkAcceleration"));
      params.putFloat("TrafficJerkDeceleration", params_default.getFloat("TrafficJerkDeceleration"));
      params.putFloat("TrafficJerkDanger", params_default.getFloat("TrafficJerkDanger"));
      params.putFloat("TrafficJerkSpeed", params_default.getFloat("TrafficJerkSpeed"));
      params.putFloat("TrafficJerkSpeedDecrease", params_default.getFloat("TrafficJerkSpeedDecrease"));

      trafficFollowToggle->refresh();
      trafficAccelerationToggle->refresh();
      trafficDecelerationToggle->refresh();
      trafficDangerToggle->refresh();
      trafficSpeedToggle->refresh();
      trafficSpeedDecreaseToggle->refresh();
    }
  });

  FrogPilotParamValueControl *aggressiveFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveFollow"]);
  FrogPilotParamValueControl *aggressiveAccelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkAcceleration"]);
  FrogPilotParamValueControl *aggressiveDecelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkDeceleration"]);
  FrogPilotParamValueControl *aggressiveDangerToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkDanger"]);
  FrogPilotParamValueControl *aggressiveSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkSpeed"]);
  FrogPilotParamValueControl *aggressiveSpeedDecreaseToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkSpeedDecrease"]);
  FrogPilotButtonsControl *aggressiveResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetAggressivePersonality"]);
  QObject::connect(aggressiveResetButton, &FrogPilotButtonsControl::buttonClicked, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重置 <b>Aggressive</b> personality?"), this)) {
      params.putFloat("AggressiveFollow", params_default.getFloat("AggressiveFollow"));
      params.putFloat("AggressiveJerkAcceleration", params_default.getFloat("AggressiveJerkAcceleration"));
      params.putFloat("AggressiveJerkDeceleration", params_default.getFloat("AggressiveJerkDeceleration"));
      params.putFloat("AggressiveJerkDanger", params_default.getFloat("AggressiveJerkDanger"));
      params.putFloat("AggressiveJerkSpeed", params_default.getFloat("AggressiveJerkSpeed"));
      params.putFloat("AggressiveJerkSpeedDecrease", params_default.getFloat("AggressiveJerkSpeedDecrease"));

      aggressiveFollowToggle->refresh();
      aggressiveAccelerationToggle->refresh();
      aggressiveDecelerationToggle->refresh();
      aggressiveDangerToggle->refresh();
      aggressiveSpeedToggle->refresh();
      aggressiveSpeedDecreaseToggle->refresh();
    }
  });

  FrogPilotParamValueControl *standardFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardFollow"]);
  FrogPilotParamValueControl *standardAccelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkAcceleration"]);
  FrogPilotParamValueControl *standardDecelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkDeceleration"]);
  FrogPilotParamValueControl *standardDangerToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkDanger"]);
  FrogPilotParamValueControl *standardSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkSpeed"]);
  FrogPilotParamValueControl *standardSpeedDecreaseToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkSpeedDecrease"]);
  FrogPilotButtonsControl *standardResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetStandardPersonality"]);
  QObject::connect(standardResetButton, &FrogPilotButtonsControl::buttonClicked, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重置 <b>Standard</b> personality?"), this)) {
      params.putFloat("StandardFollow", params_default.getFloat("StandardFollow"));
      params.putFloat("StandardJerkAcceleration", params_default.getFloat("StandardJerkAcceleration"));
      params.putFloat("StandardJerkDeceleration", params_default.getFloat("StandardJerkDeceleration"));
      params.putFloat("StandardJerkDanger", params_default.getFloat("StandardJerkDanger"));
      params.putFloat("StandardJerkSpeed", params_default.getFloat("StandardJerkSpeed"));
      params.putFloat("StandardJerkSpeedDecrease", params_default.getFloat("StandardJerkSpeedDecrease"));

      standardFollowToggle->refresh();
      standardAccelerationToggle->refresh();
      standardDecelerationToggle->refresh();
      standardDangerToggle->refresh();
      standardSpeedToggle->refresh();
      standardSpeedDecreaseToggle->refresh();
    }
  });

  FrogPilotParamValueControl *relaxedFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedFollow"]);
  FrogPilotParamValueControl *relaxedAccelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkAcceleration"]);
  FrogPilotParamValueControl *relaxedDecelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkDeceleration"]);
  FrogPilotParamValueControl *relaxedDangerToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkDanger"]);
  FrogPilotParamValueControl *relaxedSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkSpeed"]);
  FrogPilotParamValueControl *relaxedSpeedDecreaseToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkSpeedDecrease"]);
  FrogPilotButtonsControl *relaxedResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetRelaxedPersonality"]);
  QObject::connect(relaxedResetButton, &FrogPilotButtonsControl::buttonClicked, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重置 <b>Relaxed</b> personality?"), this)) {
      params.putFloat("RelaxedFollow", params_default.getFloat("RelaxedFollow"));
      params.putFloat("RelaxedJerkAcceleration", params_default.getFloat("RelaxedJerkAcceleration"));
      params.putFloat("RelaxedJerkDeceleration", params_default.getFloat("RelaxedJerkDeceleration"));
      params.putFloat("RelaxedJerkDanger", params_default.getFloat("RelaxedJerkDanger"));
      params.putFloat("RelaxedJerkSpeed", params_default.getFloat("RelaxedJerkSpeed"));
      params.putFloat("RelaxedJerkSpeedDecrease", params_default.getFloat("RelaxedJerkSpeedDecrease"));

      relaxedFollowToggle->refresh();
      relaxedAccelerationToggle->refresh();
      relaxedDecelerationToggle->refresh();
      relaxedDangerToggle->refresh();
      relaxedSpeedToggle->refresh();
      relaxedSpeedDecreaseToggle->refresh();
    }
  });

  openDescriptions(forceOpenDescriptions, toggles);

  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubPanel, [longitudinalLayout, longitudinalPanel, this] {
    openDescriptions(forceOpenDescriptions, toggles);
    longitudinalLayout->setCurrentWidget(longitudinalPanel);
  });
  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubSubPanel, [longitudinalLayout, customDrivingPersonalityPanel, qolPanel, speedLimitControllerPanel, this]() {
    openDescriptions(forceOpenDescriptions, toggles);

    if (customPersonalityOpen) {
      longitudinalLayout->setCurrentWidget(customDrivingPersonalityPanel);

      customPersonalityOpen = false;
    } else if (qolOpen) {
      longitudinalLayout->setCurrentWidget(qolPanel);

      qolOpen = false;
    } else if (slcOpen) {
      longitudinalLayout->setCurrentWidget(speedLimitControllerPanel);

      slcOpen = false;
    }
  });
  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubSubSubPanel, [longitudinalLayout, weatherPanel, this]() {
    openDescriptions(forceOpenDescriptions, toggles);

    if (weatherOpen) {
      longitudinalLayout->setCurrentWidget(weatherPanel);

      weatherOpen = false;
    }
  });
  QObject::connect(parent, &FrogPilotSettingsWindow::updateMetric, this, &FrogPilotLongitudinalPanel::updateMetric);
}

void FrogPilotLongitudinalPanel::showEvent(QShowEvent *event) {
  FrogPilotUIState &fs = *frogpilotUIState();

  frogpilotToggleLevels = parent->frogpilotToggleLevels;

  calibratedLateralAccelerationLabel->setText(QString::number(params.getFloat("CalibratedLateralAcceleration"), 'f', 2) + tr(" m/s²"));
  calibrationProgressLabel->setText(QString::number(params.getFloat("CalibrationProgress"), 'f', 2) + "%");

  longitudinalActuatorDelayToggle->setTitle(QString(tr("啟動延遲 (Default: %1)")).arg(QString::number(parent->longitudinalActuatorDelay, 'f', 2)));
  startAccelToggle->setTitle(QString(tr("起始加速度 (Default: %1)")).arg(QString::number(parent->startAccel, 'f', 2)));
  stopAccelToggle->setTitle(QString(tr("停止加速度 (Default: %1)")).arg(QString::number(parent->stopAccel, 'f', 2)));
  stoppingDecelRateToggle->setTitle(QString(tr("停止速率 (Default: %1)")).arg(QString::number(parent->stoppingDecelRate, 'f', 2)));
  vEgoStartingToggle->setTitle(QString(tr("起始速度 (Default: %1)")).arg(QString::number(parent->vEgoStarting, 'f', 2)));
  vEgoStoppingToggle->setTitle(QString(tr("停止速度 (Default: %1)")).arg(QString::number(parent->vEgoStopping, 'f', 2)));

  bool keyExists = !params.get("WeatherToken").empty();
  weatherKeyControl->setText(0, keyExists ? tr("移除") : tr("增加"));
  weatherKeyControl->setVisibleButton(1, keyExists && fs.frogpilot_scene.online);

  updateToggles();
}

void FrogPilotLongitudinalPanel::updateMetric(bool metric, bool bootRun) {
  static bool previousMetric;
  if (metric != previousMetric && !bootRun) {
    double distanceConversion = metric ? FOOT_TO_METER : METER_TO_FOOT;
    double speedConversion = metric ? MILE_TO_KM : KM_TO_MILE;

    params.putIntNonBlocking("IncreasedStoppedDistance", params.getInt("IncreasedStoppedDistance") * distanceConversion);
    params.putIntNonBlocking("IncreasedStoppedDistanceLowVisibility", params.getInt("IncreasedStoppedDistanceLowVisibility") * distanceConversion);
    params.putIntNonBlocking("IncreasedStoppedDistanceRain", params.getInt("IncreasedStoppedDistanceRain") * distanceConversion);
    params.putIntNonBlocking("IncreasedStoppedDistanceRainStorm", params.getInt("IncreasedStoppedDistanceRainStorm") * distanceConversion);
    params.putIntNonBlocking("IncreasedStoppedDistanceSnow", params.getInt("IncreasedStoppedDistanceSnow") * distanceConversion);

    params.putIntNonBlocking("CESignalSpeed", params.getInt("CESignalSpeed") * speedConversion);
    params.putIntNonBlocking("CESpeed", params.getInt("CESpeed") * speedConversion);
    params.putIntNonBlocking("CESpeedLead", params.getInt("CESpeedLead") * speedConversion);
    params.putIntNonBlocking("CustomCruise", params.getInt("CustomCruise") * speedConversion);
    params.putIntNonBlocking("CustomCruiseLong", params.getInt("CustomCruiseLong") * speedConversion);
    params.putIntNonBlocking("Offset1", params.getInt("Offset1") * speedConversion);
    params.putIntNonBlocking("Offset2", params.getInt("Offset2") * speedConversion);
    params.putIntNonBlocking("Offset3", params.getInt("Offset3") * speedConversion);
    params.putIntNonBlocking("Offset4", params.getInt("Offset4") * speedConversion);
    params.putIntNonBlocking("Offset5", params.getInt("Offset5") * speedConversion);
    params.putIntNonBlocking("Offset6", params.getInt("Offset6") * speedConversion);
    params.putIntNonBlocking("Offset7", params.getInt("Offset7") * speedConversion);
    params.putIntNonBlocking("SetSpeedOffset", params.getInt("SetSpeedOffset") * speedConversion);
  }
  previousMetric = metric;

  static std::map<float, QString> imperialDistanceLabels;
  static std::map<float, QString> imperialSpeedLabels;
  static std::map<float, QString> metricDistanceLabels;
  static std::map<float, QString> metricSpeedLabels;

  static bool labelsInitialized = false;
  if (!labelsInitialized) {
    for (int i = 0; i <= 10; ++i) {
      imperialDistanceLabels[i] = i == 0 ? tr("關閉") : i == 1 ? QString::number(i) + tr(" foot") : QString::number(i) + tr(" feet");
    }

    for (int i = 0; i <= 99; ++i) {
      imperialSpeedLabels[i] = i == 0 ? tr("關閉") : QString::number(i) + tr(" mph");
    }

    for (int i = 0; i <= 3; ++i) {
      metricDistanceLabels[i] = i == 0 ? tr("關閉") : i == 1 ? QString::number(i) + tr(" meter") : QString::number(i) + tr(" meters");
    }

    for (int i = 0; i <= 150; ++i) {
      metricSpeedLabels[i] = i == 0 ? tr("關閉") : QString::number(i) + tr(" km/h");
    }

    labelsInitialized = true;
  }

  FrogPilotDualParamValueControl *ceSpeedToggle = reinterpret_cast<FrogPilotDualParamValueControl*>(toggles["CESpeed"]);
  FrogPilotParamValueButtonControl *ceSignal = static_cast<FrogPilotParamValueButtonControl*>(toggles["CESignalSpeed"]);
  FrogPilotParamValueControl *customCruiseToggle = static_cast<FrogPilotParamValueControl*>(toggles["CustomCruise"]);
  FrogPilotParamValueControl *customCruiseLongToggle = static_cast<FrogPilotParamValueControl*>(toggles["CustomCruiseLong"]);
  FrogPilotParamValueControl *offset1Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset1"]);
  FrogPilotParamValueControl *offset2Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset2"]);
  FrogPilotParamValueControl *offset3Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset3"]);
  FrogPilotParamValueControl *offset4Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset4"]);
  FrogPilotParamValueControl *offset5Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset5"]);
  FrogPilotParamValueControl *offset6Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset6"]);
  FrogPilotParamValueControl *offset7Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset7"]);
  FrogPilotParamValueControl *increasedStoppedDistanceToggle = static_cast<FrogPilotParamValueControl*>(toggles["IncreasedStoppedDistance"]);
  FrogPilotParamValueControl *increasedStoppedDistanceLowVisibilityToggle = static_cast<FrogPilotParamValueControl*>(toggles["IncreasedStoppedDistanceLowVisibility"]);
  FrogPilotParamValueControl *increasedStoppedDistanceRainToggle = static_cast<FrogPilotParamValueControl*>(toggles["IncreasedStoppedDistanceRain"]);
  FrogPilotParamValueControl *increasedStoppedDistanceRainStormToggle = static_cast<FrogPilotParamValueControl*>(toggles["IncreasedStoppedDistanceRainStorm"]);
  FrogPilotParamValueControl *increasedStoppedDistanceSnowToggle = static_cast<FrogPilotParamValueControl*>(toggles["IncreasedStoppedDistanceSnow"]);
  FrogPilotParamValueControl *setSpeedOffsetToggle = static_cast<FrogPilotParamValueControl*>(toggles["SetSpeedOffset"]);

  if (metric) {
    offset1Toggle->setTitle(tr("速度偏移 (0–29 km/h)"));
    offset2Toggle->setTitle(tr("速度偏移 (30–49 km/h)"));
    offset3Toggle->setTitle(tr("速度偏移 (50–59 km/h)"));
    offset4Toggle->setTitle(tr("速度偏移 (60–79 km/h)"));
    offset5Toggle->setTitle(tr("速度偏移 (80–99 km/h)"));
    offset6Toggle->setTitle(tr("速度偏移 (100–119 km/h)"));
    offset7Toggle->setTitle(tr("速度偏移 (120–140 km/h)"));

    offset1Toggle->setDescription(tr("<b>在 0 到 24 英里/小時之間偏移張貼的速度限制多少</b>。"));
    offset2Toggle->setDescription(tr("<b>在 25 到 34 英里/小時之間偏移張貼的速度限制多少</b>。"));
    offset3Toggle->setDescription(tr("<b>在 35 到 44 英里/小時之間偏移張貼的速度限制多少</b>。"));
    offset4Toggle->setDescription(tr("<b>在 45 到 54 英里/小時之間偏移張貼的速度限制多少</b>。"));
    offset5Toggle->setDescription(tr("<b>在 55 到 64 英里/小時之間偏移張貼的速度限制多少</b>。"));
    offset6Toggle->setDescription(tr("<b>在 65 到 74 英里/小時之間偏移張貼的速度限制多少</b>。"));
    offset7Toggle->setDescription(tr("<b>在 75 到 99 英里/小時之間偏移張貼的速度限制多少</b>。"));

    increasedStoppedDistanceToggle->updateControl(0, 3, metricDistanceLabels);
    increasedStoppedDistanceLowVisibilityToggle->updateControl(0, 3, metricDistanceLabels);
    increasedStoppedDistanceRainToggle->updateControl(0, 3, metricDistanceLabels);
    increasedStoppedDistanceRainStormToggle->updateControl(0, 3, metricDistanceLabels);
    increasedStoppedDistanceSnowToggle->updateControl(0, 3, metricDistanceLabels);

    ceSignal->updateControl(0, 150, metricSpeedLabels);
    ceSpeedToggle->updateControl(0, 150, metricSpeedLabels);
    customCruiseToggle->updateControl(1, 150, metricSpeedLabels);
    customCruiseLongToggle->updateControl(1, 150, metricSpeedLabels);
    offset1Toggle->updateControl(-150, 150, metricSpeedLabels);
    offset2Toggle->updateControl(-150, 150, metricSpeedLabels);
    offset3Toggle->updateControl(-150, 150, metricSpeedLabels);
    offset4Toggle->updateControl(-150, 150, metricSpeedLabels);
    offset5Toggle->updateControl(-150, 150, metricSpeedLabels);
    offset6Toggle->updateControl(-150, 150, metricSpeedLabels);
    offset7Toggle->updateControl(-150, 150, metricSpeedLabels);
    setSpeedOffsetToggle->updateControl(-150, 150, metricSpeedLabels);
  } else {
    offset1Toggle->setTitle(tr("速度偏移 (0–24 英里/小時)"));
    offset2Toggle->setTitle(tr("速度偏移 (25–34 英里/小時)"));
    offset3Toggle->setTitle(tr("速度偏移 (35–44 英里/小時)"));
    offset4Toggle->setTitle(tr("速度偏移 (45–54 英里/小時)"));
    offset5Toggle->setTitle(tr("速度偏移 (55–64 英里/小時)"));
    offset6Toggle->setTitle(tr("速度偏移 (65–74 英里/小時)"));
    offset7Toggle->setTitle(tr("速度偏移 (75–99 英里/小時)"));

    offset1Toggle->setDescription(tr("<b>在 0 到 24 英里/小時之間偏移張貼的速度限制多少</b>。"));
    offset2Toggle->setDescription(tr("<b>在 25 到 34 英里/小時之間偏移張貼的速度限制多少</b>。"));
    offset3Toggle->setDescription(tr("<b>在 35 到 44 英里/小時之間偏移張貼的速度限制多少</b>。"));
    offset4Toggle->setDescription(tr("<b>在 45 到 54 英里/小時之間偏移張貼的速度限制多少</b>。"));
    offset5Toggle->setDescription(tr("<b>在 55 到 64 英里/小時之間偏移張貼的速度限制多少</b>。"));
    offset6Toggle->setDescription(tr("<b>在 65 到 74 英里/小時之間偏移張貼的速度限制多少</b>。"));
    offset7Toggle->setDescription(tr("<b>在 75 到 99 英里/小時之間偏移張貼的速度限制多少</b>。"));

    increasedStoppedDistanceToggle->updateControl(0, 10, imperialDistanceLabels);
    increasedStoppedDistanceLowVisibilityToggle->updateControl(0, 10, imperialDistanceLabels);
    increasedStoppedDistanceRainToggle->updateControl(0, 10, imperialDistanceLabels);
    increasedStoppedDistanceRainStormToggle->updateControl(0, 10, imperialDistanceLabels);
    increasedStoppedDistanceSnowToggle->updateControl(0, 10, imperialDistanceLabels);

    ceSignal->updateControl(0, 99, imperialSpeedLabels);
    ceSpeedToggle->updateControl(0, 99, imperialSpeedLabels);
    customCruiseToggle->updateControl(1, 99, imperialSpeedLabels);
    customCruiseLongToggle->updateControl(1, 99, imperialSpeedLabels);
    offset1Toggle->updateControl(-99, 99, imperialSpeedLabels);
    offset2Toggle->updateControl(-99, 99, imperialSpeedLabels);
    offset3Toggle->updateControl(-99, 99, imperialSpeedLabels);
    offset4Toggle->updateControl(-99, 99, imperialSpeedLabels);
    offset5Toggle->updateControl(-99, 99, imperialSpeedLabels);
    offset6Toggle->updateControl(-99, 99, imperialSpeedLabels);
    offset7Toggle->updateControl(-99, 99, imperialSpeedLabels);
    setSpeedOffsetToggle->updateControl(0, 99, imperialSpeedLabels);
  }
}

void FrogPilotLongitudinalPanel::updateToggles() {
  for (auto &[key, toggle] : toggles) {
    if (parentKeys.contains(key)) {
      toggle->setVisible(false);
    }
  }

  for (auto &[key, toggle] : toggles) {
    if (parentKeys.contains(key)) {
      continue;
    }

    bool setVisible = parent->tuningLevel >= frogpilotToggleLevels[key].toDouble();

    if (key == "CEStopLights") {
      setVisible &= !toggles["CEModelStopTime"]->isVisible();
    }

    else if (key == "CustomCruise" || key == "CustomCruiseLong" || key == "SetSpeedLimit" || key == "SetSpeedOffset") {
      setVisible &= !parent->hasPCMCruise;
    }

    else if (key == "ForceMPHDashboard") {
      setVisible &= parent->isToyota;
    }

    else if (key == "HumanLaneChanges") {
      setVisible &= parent->hasRadar;
    }

    else if (key == "MapGears") {
      setVisible &= parent->isGM || parent->isHKGCanFd || parent->isToyota;
      setVisible &= !parent->isTSK;
    }

    else if (key == "ReverseCruise") {
      setVisible &= parent->isToyota;
    }

    else if (key == "SLCMapboxFiller") {
      setVisible &= !params.get("MapboxSecretKey").empty();
    }

    else if (key == "StartAccel") {
      setVisible &= !(params.getBool("LongitudinalTune") && params.getBool("HumanAcceleration"));
    }

    else if (key == "StoppingDecelRate" || key == "VEgoStarting" || key == "VEgoStopping") {
      setVisible &= !parent->isGM || !params.getBool("ExperimentalGMTune");
      setVisible &= !parent->isToyota || !params.getBool("FrogsGoMoosTweak");
    }

    toggle->setVisible(setVisible);

    if (setVisible) {
      if (advancedLongitudinalTuneKeys.contains(key)) {
        toggles["AdvancedLongitudinalTune"]->setVisible(true);
      } else if (aggressivePersonalityKeys.contains(key)) {
        toggles["AggressivePersonalityProfile"]->setVisible(true);
      } else if (conditionalExperimentalKeys.contains(key)) {
        toggles["ConditionalExperimental"]->setVisible(true);
      } else if (curveSpeedKeys.contains(key)) {
        toggles["CurveSpeedController"]->setVisible(true);
      } else if (customDrivingPersonalityKeys.contains(key)) {
        toggles["CustomPersonalities"]->setVisible(true);
      } else if (longitudinalTuneKeys.contains(key)) {
        toggles["LongitudinalTune"]->setVisible(true);
      } else if (qolKeys.contains(key)) {
        toggles["QOLLongitudinal"]->setVisible(true);
      } else if (relaxedPersonalityKeys.contains(key)) {
        toggles["RelaxedPersonalityProfile"]->setVisible(true);
      } else if (speedLimitControllerKeys.contains(key)) {
        toggles["SpeedLimitController"]->setVisible(true);
      } else if (speedLimitControllerOffsetsKeys.contains(key)) {
        toggles["SLCOffsets"]->setVisible(true);
      } else if (speedLimitControllerQOLKeys.contains(key)) {
        toggles["SLCQOL"]->setVisible(true);
      } else if (speedLimitControllerVisualKeys.contains(key)) {
        toggles["SLCVisuals"]->setVisible(true);
      } else if (standardPersonalityKeys.contains(key)) {
        toggles["StandardPersonalityProfile"]->setVisible(true);
      } else if (trafficPersonalityKeys.contains(key)) {
        toggles["TrafficPersonalityProfile"]->setVisible(true);
      }
    }
  }

  openDescriptions(forceOpenDescriptions, toggles);

  update();
}
