let db;
let state;
import { ref, onValue, set, query, limitToLast } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";

let chartInstance = null;

function initChart() {
    const ctx = document.getElementById('temp-hum-chart');
    if (!ctx) return;

    const isLight = document.body.classList.contains("light-mode");
    const textColor = isLight ? '#475569' : '#94a3b8';
    const gridColor = isLight ? 'rgba(0, 0, 0, 0.04)' : 'rgba(255, 255, 255, 0.04)';

    const chartCtx = ctx.getContext('2d');
    const tempGradient = chartCtx.createLinearGradient(0, 0, 0, 200);
    tempGradient.addColorStop(0, 'rgba(239, 68, 68, 0.12)');
    tempGradient.addColorStop(1, 'rgba(239, 68, 68, 0.00)');

    const humGradient = chartCtx.createLinearGradient(0, 0, 0, 200);
    humGradient.addColorStop(0, 'rgba(14, 165, 233, 0.12)');
    humGradient.addColorStop(1, 'rgba(14, 165, 233, 0.00)');

    chartInstance = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'Nhiệt độ (°C)',
                    data: [],
                    borderColor: '#ef4444',
                    borderWidth: 2,
                    backgroundColor: tempGradient,
                    tension: 0.4,
                    fill: true,
                    pointBackgroundColor: '#ef4444',
                    pointRadius: 2.5,
                    pointHoverRadius: 5,
                    yAxisID: 'y-temp'
                },
                {
                    label: 'Độ ẩm (%)',
                    data: [],
                    borderColor: '#0ea5e9',
                    borderWidth: 2,
                    backgroundColor: humGradient,
                    tension: 0.4,
                    fill: true,
                    pointBackgroundColor: '#0ea5e9',
                    pointRadius: 2.5,
                    pointHoverRadius: 5,
                    yAxisID: 'y-hum'
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    display: true,
                    position: 'top',
                    labels: {
                        color: textColor,
                        boxWidth: 8,
                        font: { family: 'Outfit', size: 11, weight: '500' }
                    }
                },
                tooltip: {
                    backgroundColor: isLight ? 'rgba(255, 255, 255, 0.95)' : 'rgba(15, 23, 42, 0.95)',
                    titleColor: isLight ? '#0f172a' : '#f8fafc',
                    bodyColor: isLight ? '#334155' : '#cbd5e1',
                    borderColor: isLight ? 'rgba(0,0,0,0.08)' : 'rgba(255,255,255,0.08)',
                    borderWidth: 1,
                    titleFont: { family: 'Outfit', size: 13, weight: '700' },
                    bodyFont: { family: 'Outfit', size: 13 },
                    padding: 10,
                    cornerRounding: 8
                }
            },
            scales: {
                x: {
                    grid: { display: false },
                    border: { display: false },
                    ticks: {
                        color: textColor,
                        font: { family: 'Outfit', size: 11 },
                        maxTicksLimit: 6,
                        maxRotation: 0,
                        minRotation: 0
                    }
                },
                'y-temp': {
                    type: 'linear', position: 'left',
                    grid: { color: gridColor, borderDash: [5, 5] },
                    border: { display: false },
                    ticks: {
                        color: textColor,
                        font: { family: 'Outfit', size: 11 },
                        callback: function(val) { return val + '°C'; }
                    },
                    min: 15, max: 45
                },
                'y-hum': {
                    type: 'linear', position: 'right',
                    grid: { drawOnChartArea: false },
                    border: { display: false },
                    ticks: {
                        color: textColor,
                        font: { family: 'Outfit', size: 11 },
                        callback: function(val) { return val + '%'; }
                    },
                    min: 20, max: 100
                }
            }
        }
    });
}

function updateChartFromHistory(snapshot) {
    const data = snapshot.val();
    if (!data || !chartInstance) return;

    const entries = Object.values(data).sort((a, b) => a.timestamp - b.timestamp);

    const labels = [];
    const tempData = [];
    const humData = [];

    entries.forEach(entry => {
        const d = new Date(entry.timestamp);
        const label = d.getHours().toString().padStart(2, '0') + ':' + d.getMinutes().toString().padStart(2, '0');
        labels.push(label);
        tempData.push(entry.temperature);
        humData.push(entry.humidity);
    });

    chartInstance.data.labels = labels;
    chartInstance.data.datasets[0].data = tempData;
    chartInstance.data.datasets[1].data = humData;
    chartInstance.update();
}

window.addEventListener("themechanged", (e) => {
    if (chartInstance) {
        const isLight = e.detail.theme === "light";
        const textColor = isLight ? '#475569' : '#94a3b8';
        const gridColor = isLight ? 'rgba(0, 0, 0, 0.04)' : 'rgba(255, 255, 255, 0.04)';

        chartInstance.options.plugins.legend.labels.color = textColor;
        chartInstance.options.plugins.tooltip.backgroundColor = isLight ? 'rgba(255, 255, 255, 0.95)' : 'rgba(15, 23, 42, 0.95)';
        chartInstance.options.plugins.tooltip.titleColor = isLight ? '#0f172a' : '#f8fafc';
        chartInstance.options.plugins.tooltip.bodyColor = isLight ? '#334155' : '#cbd5e1';
        chartInstance.options.plugins.tooltip.borderColor = isLight ? 'rgba(0,0,0,0.08)' : 'rgba(255,255,255,0.08)';

        chartInstance.options.scales.x.ticks.color = textColor;
        chartInstance.options.scales['y-temp'].ticks.color = textColor;
        chartInstance.options.scales['y-temp'].grid.color = gridColor;
        chartInstance.options.scales['y-hum'].ticks.color = textColor;

        chartInstance.update();
    }
});

function updateSensorUI(data) {
    if (data.temperature !== undefined) state.sensors.temperature = data.temperature;
    if (data.humidity !== undefined) state.sensors.humidity = data.humidity;
    if (data.rain !== undefined) state.sensors.rain = data.rain;
    if (data.light !== undefined) state.sensors.light = data.light;

    const temp = state.sensors.temperature;
    const hum = state.sensors.humidity;
    const rain = state.sensors.rain;
    const light = state.sensors.light;

    const tempEl = document.getElementById("temp-val");
    const humEl = document.getElementById("hum-val");
    const tempGaugeCircular = document.getElementById("temp-gauge-circular");
    const humGaugeCircular = document.getElementById("hum-gauge-circular");
    const circumferenceCircular = Math.PI * 45 * 1.5;

    if (tempEl) tempEl.innerText = temp !== null ? Math.round(temp) : "--";
    if (humEl) humEl.innerText = hum !== null ? Math.round(hum) : "--";

    if (tempGaugeCircular) {
        const progress = temp !== null ? Math.max(0, Math.min(temp / 50, 1)) : 0;
        tempGaugeCircular.style.strokeDasharray = circumferenceCircular;
        tempGaugeCircular.style.strokeDashoffset = circumferenceCircular * (1 - progress);
    }
    if (humGaugeCircular) {
        const progress = hum !== null ? Math.max(0, Math.min(hum / 100, 1)) : 0;
        humGaugeCircular.style.strokeDasharray = circumferenceCircular;
        humGaugeCircular.style.strokeDashoffset = circumferenceCircular * (1 - progress);
    }

    const rainIcon = document.getElementById("rain-icon");
    const rainStatus = document.getElementById("rain-status");
    if (rainIcon && rainStatus) {
        if (rain == 1) {
            rainIcon.innerHTML = '<i class="fa-solid fa-cloud-rain" style="color: #38bdf8;"></i>';
            rainStatus.innerText = "Đang mưa";
            rainStatus.style.color = "var(--info)";
        } else {
            rainIcon.innerHTML = '<i class="fa-solid fa-sun" style="color: #f59e0b;"></i>';
            rainStatus.innerText = "Không mưa";
            rainStatus.style.color = "var(--warning)";
        }
    }

    const lightVal = document.getElementById("light-val");
    const lightDesc = document.getElementById("light-desc");
    if (lightVal) lightVal.innerText = light !== null ? Math.round(light) : "--";
    if (lightDesc) {
        if (light === null) lightDesc.innerText = "Cảm biến ánh sáng";
        else if (light < 50) lightDesc.innerText = "Tối";
        else if (light < 500) lightDesc.innerText = "Ánh sáng yếu";
        else if (light < 2000) lightDesc.innerText = "Ánh sáng tốt";
        else lightDesc.innerText = "Rất sáng";
    }

    const roofWarning = document.getElementById("roof-warning");
    if (roofWarning && rain == 1) {
        roofWarning.style.display = "flex";
        if (state.devices.roof && state.devices.roof.mode === "auto" && state.devices.roof.status !== "closed") {
            roofWarning.innerHTML = '<i class="fa-solid fa-triangle-exclamation"></i><span>Phát hiện mưa — đang tự động đóng mái che!</span>';
        }
    } else if (roofWarning) {
        roofWarning.style.display = "none";
    }
}

export function initDashboard(database, globalState) {
    db = database;
    state = globalState;

    onValue(ref(db, "sensors"), (snapshot) => {
        const data = snapshot.val();
        if (!data) return;
        updateSensorUI(data);
    });

    onValue(query(ref(db, "sensors/history"), limitToLast(288)), (snapshot) => {
        updateChartFromHistory(snapshot);
    });

    onValue(ref(db, "devices"), (snapshot) => {
        const data = snapshot.val();
        if (!data) return;

        state.devices = data;

        updateIndoorLight(data);
        updateOutdoorLight(data);
        updateDoor(data);
        updateRoof(data);
        updateFan(data);
    });

    function updateIndoorLight(data) {
        const device = data.indoor_light || { status: false };
        const toggle = document.getElementById("indoor-light-toggle");
        if (toggle) {
            toggle.checked = device.status;
            const card = toggle.closest(".device-card");
            if (card) card.classList.toggle("active", device.status);
        }
    }

    function updateOutdoorLight(data) {
        const device = data.outdoor_light || { status: false, mode: "manual" };
        const toggle = document.getElementById("outdoor-light-toggle");
        if (toggle) {
            toggle.checked = device.status;
            const card = toggle.closest(".device-card");
            if (card) card.classList.toggle("active", device.status);
        }

        const manualBtn = document.getElementById("ol-mode-manual");
        const autoBtn = document.getElementById("ol-mode-auto");
        if (manualBtn && autoBtn) {
            manualBtn.classList.toggle("active", device.mode === "manual");
            autoBtn.classList.toggle("active", device.mode === "auto");
        }
    }

    function updateDoor(data) {
        const device = data.door || {};
        const status = device.status || "closed";
        const autoCloseMs = device.auto_close_ms !== undefined ? device.auto_close_ms : 5000;

        const badge = document.getElementById("door-status-badge");
        const icon = document.getElementById("door-icon");
        if (badge && icon) {
            const card = badge.closest(".device-card");
            if (status === "open") {
                badge.innerText = "Mở";
                badge.className = "badge open";
                icon.className = "fa-solid fa-lock-open device-icon";
                if (card) card.classList.add("active");
            } else {
                badge.innerText = "Đóng";
                badge.className = "badge closed";
                icon.className = "fa-solid fa-lock device-icon";
                if (card) card.classList.remove("active");
            }
        }

        const seconds = Math.round(autoCloseMs / 1000);
        const rangeInput = document.getElementById("door-autoclose-range");
        const rangeLbl = document.getElementById("door-autoclose-lbl");
        if (rangeInput) rangeInput.value = seconds;
        if (rangeLbl) rangeLbl.innerText = seconds + "s";
    }

    function updateRoof(data) {
        const device = data.roof || { status: "closed", mode: "auto" };
        const badge = document.getElementById("roof-status-badge");
        if (badge) {
            const card = badge.closest(".device-card");
            if (device.status === "open") {
                badge.innerText = "Mở";
                badge.className = "badge open";
                if (card) card.classList.add("active");
            } else {
                badge.innerText = "Đóng";
                badge.className = "badge closed";
                if (card) card.classList.remove("active");
            }
        }

        const manualBtn = document.getElementById("roof-mode-manual");
        const autoBtn = document.getElementById("roof-mode-auto");
        if (manualBtn && autoBtn) {
            manualBtn.classList.toggle("active", device.mode === "manual");
            autoBtn.classList.toggle("active", device.mode === "auto");
        }

        if (state.sensors.rain == 1 && device.mode === "auto") {
            const warning = document.getElementById("roof-warning");
            if (warning) {
                warning.style.display = "flex";
                warning.innerHTML = '<i class="fa-solid fa-triangle-exclamation"></i><span>Phát hiện mưa — đang tự động đóng mái che!</span>';
            }
        }
    }

    function updateFan(data) {
        const device = data.fan || { status: false, speed: 50 };
        const toggle = document.getElementById("fan-toggle");
        if (toggle) {
            toggle.checked = device.status;
            const card = toggle.closest(".device-card");
            if (card) card.classList.toggle("active", device.status);
        }

        const speedRange = document.getElementById("fan-speed-range");
        const speedLbl = document.getElementById("fan-speed-lbl");
        if (speedRange) speedRange.value = device.speed || 0;
        if (speedLbl) speedLbl.innerText = (device.speed || 0) + "%";
    }

    const inLightToggle = document.getElementById("indoor-light-toggle");
    if (inLightToggle) {
        inLightToggle.addEventListener("change", (e) => {
            set(ref(db, "devices/indoor_light/status"), e.target.checked);
        });
    }

    const olToggle = document.getElementById("outdoor-light-toggle");
    if (olToggle) {
        olToggle.addEventListener("change", (e) => {
            set(ref(db, "devices/outdoor_light/status"), e.target.checked);
        });
    }

    document.getElementById("ol-mode-manual")?.addEventListener("click", () => {
        set(ref(db, "devices/outdoor_light/mode"), "manual");
    });
    document.getElementById("ol-mode-auto")?.addEventListener("click", () => {
        set(ref(db, "devices/outdoor_light/mode"), "auto");
    });

    document.getElementById("roof-open-btn")?.addEventListener("click", () => {
        set(ref(db, "devices/roof/status"), "open");
    });
    document.getElementById("roof-close-btn")?.addEventListener("click", () => {
        set(ref(db, "devices/roof/status"), "closed");
    });

    document.getElementById("roof-mode-manual")?.addEventListener("click", () => {
        set(ref(db, "devices/roof/mode"), "manual");
    });
    document.getElementById("roof-mode-auto")?.addEventListener("click", () => {
        set(ref(db, "devices/roof/mode"), "auto");
    });

    document.getElementById("door-open-btn")?.addEventListener("click", () => {
        set(ref(db, "devices/door/status"), "open");
    });
    document.getElementById("door-close-btn")?.addEventListener("click", () => {
        set(ref(db, "devices/door/status"), "closed");
    });

    const doorRangeInput = document.getElementById("door-autoclose-range");
    if (doorRangeInput) {
        doorRangeInput.addEventListener("input", (e) => {
            const val = parseInt(e.target.value);
            const rangeLbl = document.getElementById("door-autoclose-lbl");
            if (rangeLbl) rangeLbl.innerText = val + "s";
        });
        doorRangeInput.addEventListener("change", (e) => {
            const ms = parseInt(e.target.value) * 1000;
            set(ref(db, "devices/door/auto_close_ms"), ms);
        });
    }

    const fanToggle = document.getElementById("fan-toggle");
    if (fanToggle) {
        fanToggle.addEventListener("change", (e) => {
            set(ref(db, "devices/fan/status"), e.target.checked);
        });
    }

    const fanSpeedRange = document.getElementById("fan-speed-range");
    if (fanSpeedRange) {
        fanSpeedRange.addEventListener("input", (e) => {
            const val = parseInt(e.target.value);
            const speedLbl = document.getElementById("fan-speed-lbl");
            if (speedLbl) speedLbl.innerText = val + "%";
        });
        fanSpeedRange.addEventListener("change", (e) => {
            set(ref(db, "devices/fan/speed"), parseInt(e.target.value));
        });
    }

    const filterBtns = document.querySelectorAll("#device-filter-tabs .device-filter-btn");
    filterBtns.forEach(btn => {
        btn.addEventListener("click", () => {
            filterBtns.forEach(b => b.classList.remove("active"));
            btn.classList.add("active");
            const filter = btn.dataset.filter;
            document.querySelectorAll("#devices-container .device-card").forEach(card => {
                if (filter === "all" || card.dataset.zone === filter) {
                    card.style.display = "";
                } else {
                    card.style.display = "none";
                }
            });
        });
    });

    initChart();
}
