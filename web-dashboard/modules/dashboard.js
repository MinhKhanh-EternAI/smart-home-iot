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
                        font: {
                            family: 'Outfit',
                            size: 11,
                            weight: '500'
                        }
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
                    type: 'linear',
                    position: 'left',
                    grid: {
                        color: gridColor,
                        borderDash: [5, 5]
                    },
                    border: { display: false },
                    ticks: {
                        color: textColor,
                        font: { family: 'Outfit', size: 11 },
                        callback: function(val) { return val + '°C'; }
                    },
                    min: 15,
                    max: 45
                },
                'y-hum': {
                    type: 'linear',
                    position: 'right',
                    grid: { drawOnChartArea: false },
                    border: { display: false },
                    ticks: {
                        color: textColor,
                        font: { family: 'Outfit', size: 11 },
                        callback: function(val) { return val + '%'; }
                    },
                    min: 20,
                    max: 100
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

export function initDashboard(database, globalState) {
    db = database;
    state = globalState;

    // Đọc giá trị Cảm biến (cập nhật gauge)
    onValue(ref(db, "sensors"), (snapshot) => {
        const data = snapshot.val();
        if (!data) return;

        if (data.temperature !== undefined) state.sensors.temperature = data.temperature;
        else if (data.temp !== undefined) state.sensors.temperature = data.temp;

        if (data.humidity !== undefined) state.sensors.humidity = data.humidity;

        const tempEl = document.getElementById("temp-val");
        const humEl = document.getElementById("hum-val");
        const tempGaugeCircular = document.getElementById("temp-gauge-circular");
        const humGaugeCircular = document.getElementById("hum-gauge-circular");
        const circumferenceCircular = Math.PI * 45 * 1.5;

        const temp = (state.sensors.temperature !== null && state.sensors.temperature !== undefined) ? parseFloat(state.sensors.temperature) : null;
        const hum = (state.sensors.humidity !== null && state.sensors.humidity !== undefined) ? parseFloat(state.sensors.humidity) : null;

        if (tempEl) tempEl.innerText = temp !== null ? Math.round(temp).toString() : "--";
        if (humEl) humEl.innerText = hum !== null ? Math.round(hum).toString() : "--";

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
    });

    // Đọc lịch sử cảm biến (cập nhật biểu đồ)
    onValue(query(ref(db, "sensors/history"), limitToLast(288)), (snapshot) => {
        updateChartFromHistory(snapshot);
    });

    // Đọc trạng thái thiết bị
    onValue(ref(db, "devices"), (snapshot) => {
        const data = snapshot.val();
        if (!data) return;

        state.devices = data;

        // --- Đèn Trong Nhà ---
        const inLight = data.indoor_light || { status: false };
        const inLightToggle = document.getElementById("indoor-light-toggle");
        if (inLightToggle) {
            inLightToggle.checked = inLight.status;
            const card = inLightToggle.closest(".device-card");
            if (card) card.classList.toggle("active", inLight.status);
        }

        // --- Cửa Thông Minh ---
        const door = data.door || {};
        const doorStatus = door.status || "closed";
        const autoCloseMs = door.auto_close_ms !== undefined ? door.auto_close_ms : 15000;

        const doorBadge = document.getElementById("door-status-badge");
        const doorIcon = document.getElementById("door-icon");
        if (doorBadge && doorIcon) {
            const doorCard = doorBadge.closest(".device-card");
            if (doorStatus === "open") {
                doorBadge.innerText = "Mở";
                doorBadge.className = "badge open";
                doorIcon.className = "fa-solid fa-lock-open device-icon";
                if (doorCard) doorCard.classList.add("active");
            } else {
                doorBadge.innerText = "Đóng";
                doorBadge.className = "badge closed";
                doorIcon.className = "fa-solid fa-lock device-icon";
                if (doorCard) doorCard.classList.remove("active");
            }
        }

        const seconds = Math.round(autoCloseMs / 1000);
        const rangeInput = document.getElementById("door-autoclose-range");
        const rangeLbl = document.getElementById("door-autoclose-lbl");
        if (rangeInput) rangeInput.value = seconds;
        if (rangeLbl) rangeLbl.innerText = seconds + "s";
    });

    // ==========================================
    // Event Listeners
    // ==========================================
    const inLightToggle = document.getElementById("indoor-light-toggle");
    if (inLightToggle) {
        inLightToggle.addEventListener("change", (e) => {
            set(ref(db, "devices/indoor_light/status"), e.target.checked);
        });
    }

    const doorOpenBtn = document.getElementById("door-open-btn");
    const doorCloseBtn = document.getElementById("door-close-btn");
    if (doorOpenBtn) {
        doorOpenBtn.addEventListener("click", () => {
            set(ref(db, "devices/door/status"), "open");
        });
    }
    if (doorCloseBtn) {
        doorCloseBtn.addEventListener("click", () => {
            set(ref(db, "devices/door/status"), "closed");
        });
    }

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

    // Khởi tạo đồ thị
    initChart();
}
