let db;
let state;
import { ref, onValue, set, runTransaction } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";

function getVietnameseDayOfWeek() {
    const days = ["Chủ Nhật", "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy"];
    const now = new Date();
    const dayName = days[now.getDay()];
    return `hôm nay (${dayName})`;
}

function scrollToSelected(container, val) {
    const selected = container.querySelector(`.time-item-premium[data-val="${val}"]`);
    if (selected) {
        container.scrollTop = selected.offsetTop - container.offsetTop - 40;
    }
}

function initPremiumTimePickers(onTimeChange) {
    const pickers = document.querySelectorAll(".custom-time-picker");

    pickers.forEach(picker => {
        const display = picker.querySelector(".time-display-premium");
        const dropdown = picker.querySelector(".time-dropdown-premium");
        const hoursCol = picker.querySelector(".hours-col");
        const minutesCol = picker.querySelector(".minutes-col");
        const confirmBtn = picker.querySelector(".time-confirm-btn-premium");
        const textValEl = picker.querySelector(".time-text-large");
        
        // Find card parent for footer text updates (no individual footers in premium layout)
        const cardItem = picker.closest(".schedule-box-premium");
        const footerTextEl = null;
        const actionType = picker.id.includes("-on-") ? "on" : "off";

        if (!hoursCol || !minutesCol) return;

        // Clear contents
        hoursCol.innerHTML = "";
        minutesCol.innerHTML = "";

        // Generate hours
        for (let h = 0; h < 24; h++) {
            const hStr = h.toString().padStart(2, '0');
            const item = document.createElement("div");
            item.className = "time-item-premium";
            item.innerText = hStr;
            item.dataset.val = hStr;
            hoursCol.appendChild(item);
        }

        // Generate minutes
        for (let m = 0; m < 60; m++) {
            const mStr = m.toString().padStart(2, '0');
            const item = document.createElement("div");
            item.className = "time-item-premium";
            item.innerText = mStr;
            item.dataset.val = mStr;
            minutesCol.appendChild(item);
        }

        let selectedHour = "00";
        let selectedMin = "00";

        function setSelection(hour, min) {
            selectedHour = hour;
            selectedMin = min;

            // Highlight selected items
            hoursCol.querySelectorAll(".time-item-premium").forEach(item => {
                item.classList.toggle("selected", item.dataset.val === hour);
            });
            minutesCol.querySelectorAll(".time-item-premium").forEach(item => {
                item.classList.toggle("selected", item.dataset.val === min);
            });

            // Scroll to selection
            scrollToSelected(hoursCol, hour);
            scrollToSelected(minutesCol, min);
        }

        // Toggle dropdown display
        display.addEventListener("click", (e) => {
            e.stopPropagation();
            document.querySelectorAll(".custom-time-picker").forEach(p => {
                if (p !== picker) p.classList.remove("open");
            });
            picker.classList.toggle("open");
            if (picker.classList.contains("open")) {
                scrollToSelected(hoursCol, selectedHour);
                scrollToSelected(minutesCol, selectedMin);
            }
        });

        // Click item selection
        hoursCol.addEventListener("click", (e) => {
            if (e.target.classList.contains("time-item-premium")) {
                setSelection(e.target.dataset.val, selectedMin);
            }
        });

        minutesCol.addEventListener("click", (e) => {
            if (e.target.classList.contains("time-item-premium")) {
                setSelection(selectedHour, e.target.dataset.val);
            }
        });

        // Click confirm
        if (!confirmBtn) return;
        confirmBtn.addEventListener("click", (e) => {
            e.stopPropagation();
            const newVal = `${selectedHour}:${selectedMin}`;
            picker.dataset.value = newVal;
            if (textValEl) textValEl.innerText = newVal;
            
            // Update footer
            if (footerTextEl) {
                const icon = actionType === "on" ? '<i class="fa-solid fa-sun"></i>' : '<i class="fa-solid fa-moon"></i>';
                const label = actionType === "on" ? "bật" : "tắt";
                footerTextEl.innerHTML = `${icon} Đèn sẽ ${label} lúc ${newVal}`;
            }

            picker.classList.remove("open");

            if (onTimeChange) {
                onTimeChange(picker.id, newVal);
            }
        });

        // Bind standard API hooks so we can update selection dynamically from outside
        const apiKey = picker.id.replace(/-/g, "_");
        window[`updatePicker_${apiKey}`] = (timeStr) => {
            picker.dataset.value = timeStr;
            if (textValEl) textValEl.innerText = timeStr;
            
            // Update footer
            if (footerTextEl) {
                const icon = actionType === "on" ? '<i class="fa-solid fa-sun"></i>' : '<i class="fa-solid fa-moon"></i>';
                const label = actionType === "on" ? "bật" : "tắt";
                footerTextEl.innerHTML = `${icon} Đèn sẽ ${label} lúc ${timeStr}`;
            }

            const parts = timeStr.split(":");
            setSelection(parts[0], parts[1]);
        };

        // Also connect the header toggle button
        const headerToggleBtn = cardItem ? cardItem.querySelector(".sched-time-toggle-btn") : null;
        if (headerToggleBtn) {
            headerToggleBtn.addEventListener("click", (e) => {
                e.stopPropagation();
                display.click();
            });
        }
    });

    // Global click listener to close open pickers
    document.addEventListener("click", () => {
        pickers.forEach(p => p.classList.remove("open"));
    });
}

let chartInstance = null;
const chartMaxPoints = 24;
const chartData = {
    labels: [],
    temp: [],
    hum: []
};

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
            labels: chartData.labels,
            datasets: [
                {
                    label: 'Nhiệt độ (°C)',
                    data: chartData.temp,
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
                    data: chartData.hum,
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
                    grid: {
                        display: false
                    },
                    border: {
                        display: false
                    },
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
                    border: {
                        display: false
                    },
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
                    grid: {
                        drawOnChartArea: false
                    },
                    border: {
                        display: false
                    },
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

function prefillHourlyData(currentTemp, currentHum) {
    chartData.labels.length = 0;
    chartData.temp.length = 0;
    chartData.hum.length = 0;
    
    const now = new Date();
    // Prefill 24 points: from 23 hours ago to current hour
    for (let i = 23; i >= 0; i--) {
        const d = new Date(now.getTime() - i * 60 * 60 * 1000);
        const hourStr = d.getHours().toString().padStart(2, '0') + ':00';
        
        // Generate mock data centered around current values
        let mockTemp = currentTemp !== null ? currentTemp + Math.round(Math.sin((d.getHours() - 6) / 3) * 3 + (Math.random() - 0.5) * 2) : 25;
        let mockHum = currentHum !== null ? currentHum + Math.round(Math.cos((d.getHours() - 6) / 3) * 8 + (Math.random() - 0.5) * 4) : 60;
        
        mockTemp = Math.max(15, Math.min(45, Math.round(mockTemp)));
        mockHum = Math.max(20, Math.min(100, Math.round(mockHum)));
        
        chartData.labels.push(hourStr);
        chartData.temp.push(mockTemp);
        chartData.hum.push(mockHum);
    }
}

function updateChartData(temp, hum) {
    if (temp === null || hum === null) return;
    
    // Ensure chart values are always integers
    const roundedTemp = Math.round(temp);
    const roundedHum = Math.round(hum);
    
    if (chartData.labels.length === 0) {
        prefillHourlyData(roundedTemp, roundedHum);
        if (chartInstance) {
            chartInstance.update();
        } else {
            initChart();
        }
        return;
    }
    
    const now = new Date();
    const currentHourStr = now.getHours().toString().padStart(2, '0') + ':00';
    const lastIndex = chartData.labels.length - 1;
    
    if (chartData.labels[lastIndex] === currentHourStr) {
        // Still in the same hour, update the current point with latest rounded values
        chartData.temp[lastIndex] = roundedTemp;
        chartData.hum[lastIndex] = roundedHum;
    } else {
        // A new hour has started! Add new point
        chartData.labels.push(currentHourStr);
        chartData.temp.push(roundedTemp);
        chartData.hum.push(roundedHum);
        
        if (chartData.labels.length > chartMaxPoints) {
            chartData.labels.shift();
            chartData.temp.shift();
            chartData.hum.shift();
        }
    }
    
    if (chartInstance) {
        chartInstance.update();
    } else {
        initChart();
    }
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
    const DAY_NAMES = ["sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"];
    const VIET_DAY_NAMES = ["CN", "T2", "T3", "T4", "T5", "T6", "T7"];

    function updateDayPicker(pickerId, daysObj, disabled, dayNames = DAY_NAMES) {
        const picker = document.getElementById(pickerId);
        if (!picker) return;
        picker.querySelectorAll(".day-btn").forEach(btn => {
            const dayKey = dayNames[parseInt(btn.dataset.day)];
            btn.classList.toggle("active", daysObj && daysObj[dayKey] === true);
        });
        picker.classList.toggle("disabled", !!disabled);
    }

    function initDayPicker(pickerId, fbPath, dayNames = DAY_NAMES) {
        const picker = document.getElementById(pickerId);
        if (!picker) return;
        picker.addEventListener("click", (e) => {
            const btn = e.target.closest(".day-btn");
            if (!btn) return;
            const dayKey = dayNames[parseInt(btn.dataset.day)];
            btn.classList.toggle("active");
            runTransaction(ref(db, `${fbPath}/${dayKey}`), (currentVal) => {
                return currentVal === true ? false : true;
            }).catch(() => {
                btn.classList.toggle("active");
            });
        });
    }
    // Đọc giá trị Cảm biến
    const updateSensorsUI = (data) => {
        if (!data) return;

        // Cập nhật state toàn cục
        if (data.rain !== undefined) state.sensors.rain = !!data.rain;
        
        if (data.temperature !== undefined) state.sensors.temperature = data.temperature;
        else if (data.temp !== undefined) state.sensors.temperature = data.temp;
        
        if (data.humidity !== undefined) state.sensors.humidity = data.humidity;

        // Cập nhật Nhiệt độ / Độ ẩm + tiến trình vòm tròn
        const tempEl = document.getElementById("temp-val");
        const humEl = document.getElementById("hum-val");
        const tempGaugeCircular = document.getElementById("temp-gauge-circular");
        const humGaugeCircular = document.getElementById("hum-gauge-circular");
        const circumferenceCircular = Math.PI * 45 * 1.5; // Chu vi vòm tròn 270 độ r=45

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

        // Cập nhật biểu đồ (Chart) nếu có dữ liệu hợp lệ
        if (temp !== null && hum !== null) {
            updateChartData(temp, hum);
        }

    };

    onValue(ref(db, "sensors"), (snapshot) => {
        const val = snapshot.val();
        if (val) {
            updateSensorsUI(val);
        } else {
            // Mock data để test giao diện
            const mockData = {
                temperature: 25.5,
                humidity: 65,
                rain: true,
                last_updated: Date.now()
            };
            set(ref(db, "sensors"), mockData);
        }
    });

    // Đọc và đồng bộ các Thiết bị (Devices)
    onValue(ref(db, "devices"), (snapshot) => {
        const data = snapshot.val();
        if (!data) {
            // Mock devices data để test giao diện
            const mockDevices = {
                indoor_light: { status: true, mode: "manual", schedule: { enabled: true, on_time: "18:00", off_time: "06:00", days: { monday: true, tuesday: true, wednesday: true, thursday: true, friday: true, saturday: true, sunday: true } } },
                outdoor_light: { status: false, mode: "auto" },
                door: { status: "closed", auto_close_ms: 5000 },
                roof: { status: "open", mode: "auto" },
                fan: { status: false, schedule: { enabled: false, on_time: "12:00", off_time: "14:00", days: { monday: true, tuesday: true, wednesday: true, thursday: true, friday: true, saturday: true, sunday: true } } }
            };
            set(ref(db, "devices"), mockDevices);
            return;
        }

        // Cập nhật state toàn cục
        state.devices = data;

        // --- Đèn Trong Nhà ---
        const inLight = data.indoor_light || { status: false, mode: "manual", schedule: { enabled: false, on_time: "18:00", off_time: "06:00" } };
        const inLightToggle = document.getElementById("indoor-light-toggle");
        if (inLightToggle) {
            inLightToggle.checked = inLight.status;
            const card = inLightToggle.closest(".device-card");
            if (card) card.classList.toggle("active", inLight.status);
        }

        const inScheduleSection = document.getElementById("indoor-schedule-section");
        if (inScheduleSection) {
            inScheduleSection.style.display = "block";
        }
        
        // Hẹn giờ Đèn trong nhà
        if (inLight.schedule) {
            const enableCheck = document.getElementById("indoor-sched-enable");
            if (enableCheck) enableCheck.checked = inLight.schedule.enabled;
            
            // Cập nhật bộ chọn giờ mới
            if (window.updatePicker_indoor_on_time) window.updatePicker_indoor_on_time(inLight.schedule.on_time || "18:00");
            if (window.updatePicker_indoor_off_time) window.updatePicker_indoor_off_time(inLight.schedule.off_time || "06:00");

            // UX: Vô hiệu hóa ô chọn giờ và day picker nếu không bật hẹn giờ
            if (inScheduleSection) {
                const indoorSchedPickers = inScheduleSection.querySelector(".schedule-picker-grid-premium");
                if (indoorSchedPickers) {
                    indoorSchedPickers.classList.toggle("disabled", !inLight.schedule.enabled);
                }
            }
            const inDays = inLight.schedule.days || { monday: true, tuesday: true, wednesday: true, thursday: true, friday: true, saturday: true, sunday: true };
            updateDayPicker("indoor-day-picker", inDays, !inLight.schedule.enabled, DAY_NAMES);

            // Hiển thị trạng thái lịch trình tiếp theo
            const indoorNextActionText = document.getElementById("indoor-next-action-text");
            const indoorIndicator = document.getElementById("indoor-indicator-dot");
            if (inScheduleSection) {
                inScheduleSection.classList.toggle("active", inLight.schedule.enabled);
            }
            if (inLight.schedule.enabled) {
                const nextTime = inLight.status ? (inLight.schedule.off_time || "06:00") : (inLight.schedule.on_time || "18:00");
                const nextAct = inLight.status ? "Tắt" : "Bật";
                if (indoorNextActionText) {
                    indoorNextActionText.innerHTML = `Lịch trình tiếp theo: <strong>${nextAct} lúc ${nextTime}</strong>`;
                }
                if (indoorIndicator) {
                    indoorIndicator.classList.add("active");
                }
            } else {
                if (indoorNextActionText) {
                    indoorNextActionText.innerText = "Chưa kích hoạt lịch trình";
                }
                if (indoorIndicator) {
                    indoorIndicator.classList.remove("active");
                }
            }
        }

        // --- Đèn Ngoài Sân ---
        const outLight = data.outdoor_light || { status: false, mode: "auto" };
        const outLightToggle = document.getElementById("outdoor-light-toggle");
        if (outLightToggle) {
            outLightToggle.checked = outLight.status;
            const card = outLightToggle.closest(".device-card");
            if (card) card.classList.toggle("active", outLight.status);
            
            // UX: Vô hiệu hóa công tắc điều khiển thủ công khi ở chế độ Tự động (PIR)
            const isAuto = (outLight.mode === "auto");
            outLightToggle.disabled = isAuto;
            const sw = outLightToggle.closest(".switch");
            if (sw) sw.classList.toggle("disabled", isAuto);
        }
        
        // Chế độ Đèn Ngoài Sân (Auto/Manual)
        const outModeAuto = document.getElementById("out-mode-auto");
        const outModeManual = document.getElementById("out-mode-manual");
        const outAutoInfo = document.getElementById("outdoor-auto-info");
        const outManualInfo = document.getElementById("outdoor-manual-info");
        const isAuto = (outLight.mode === "auto");
        if (isAuto) {
            if (outModeAuto) outModeAuto.classList.add("active");
            if (outModeManual) outModeManual.classList.remove("active");
            if (outAutoInfo) outAutoInfo.style.display = "flex";
            if (outManualInfo) outManualInfo.style.display = "none";
        } else {
            if (outModeAuto) outModeAuto.classList.remove("active");
            if (outModeManual) outModeManual.classList.add("active");
            if (outAutoInfo) outAutoInfo.style.display = "none";
            if (outManualInfo) outManualInfo.style.display = "flex";
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
                doorIcon.className = "fa-solid fa-lock device-icon";
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


        // --- Mái Che ---
        const roof = data.roof || { status: "open", mode: "auto" };
        const roofBadge = document.getElementById("roof-status-badge");
        const roofIcon = document.getElementById("roof-icon");
        if (roofBadge && roofIcon) {
            const roofCard = roofBadge.closest(".device-card");
            const roofModeAuto = document.getElementById("roof-mode-auto");
            const roofModeManual = document.getElementById("roof-mode-manual");
            const roofManualRow = document.getElementById("roof-manual-controls");
            const roofAutoInfo = document.getElementById("roof-auto-info");

            if (roof.status === "open") {
                roofBadge.innerText = "Mở";
                roofBadge.className = "badge open";
                roofIcon.className = "fa-solid fa-house device-icon";
                if (roofCard) roofCard.classList.add("active");
            } else {
                roofBadge.innerText = "Đóng";
                roofBadge.className = "badge closed";
                roofIcon.className = "fa-solid fa-house device-icon";
                if (roofCard) roofCard.classList.remove("active");
            }

            if (roof.mode === "auto") {
                if (roofModeAuto) roofModeAuto.classList.add("active");
                if (roofModeManual) roofModeManual.classList.remove("active");
                if (roofManualRow) roofManualRow.style.display = "none";
                if (roofAutoInfo) roofAutoInfo.style.display = "flex";
            } else {
                if (roofModeAuto) roofModeAuto.classList.remove("active");
                if (roofModeManual) roofModeManual.classList.add("active");
                if (roofManualRow) roofManualRow.style.display = "flex";
                if (roofAutoInfo) roofAutoInfo.style.display = "none";
            }
        }

    });

    // Đọc trạng thái Quạt từ Firebase
    onValue(ref(db, "devices/fan"), (snapshot) => {
        const data = snapshot.val();
        if (!data) return;

        const fanOn = (data.status === true);
        const toggle = document.getElementById("fan-toggle");
        const card = document.getElementById("fan-card-container");

        if (toggle) toggle.checked = fanOn;
        if (card) card.classList.toggle("active", fanOn);

        // Cập nhật hẹn giờ Quạt
        const fanScheduleSection = document.getElementById("fan-schedule-section");
        if (data.schedule) {
            const enableCheck = document.getElementById("fan-sched-enable");
            if (enableCheck) enableCheck.checked = data.schedule.enabled;

            // Cập nhật bộ chọn giờ mới
            if (window.updatePicker_fan_on_time) window.updatePicker_fan_on_time(data.schedule.on_time || "12:00");
            if (window.updatePicker_fan_off_time) window.updatePicker_fan_off_time(data.schedule.off_time || "14:00");

            // UX: Vô hiệu hóa ô chọn giờ và day picker nếu không bật hẹn giờ
            if (fanScheduleSection) {
                const fanSchedPickers = fanScheduleSection.querySelector(".schedule-picker-grid-premium");
                if (fanSchedPickers) {
                    fanSchedPickers.classList.toggle("disabled", !data.schedule.enabled);
                }
                fanScheduleSection.style.display = "block";
                fanScheduleSection.classList.toggle("active", data.schedule.enabled);
            }

            const fanDays = data.schedule.days || { monday: true, tuesday: true, wednesday: true, thursday: true, friday: true, saturday: true, sunday: true };
            updateDayPicker("fan-day-picker", fanDays, !data.schedule.enabled, DAY_NAMES);

            // Hiển thị trạng thái lịch trình tiếp theo
            const fanNextActionText = document.getElementById("fan-next-action-text");
            const fanIndicator = document.getElementById("fan-indicator-dot");

            if (data.schedule.enabled) {
                const nextTime = fanOn ? (data.schedule.off_time || "14:00") : (data.schedule.on_time || "12:00");
                const nextAct = fanOn ? "Tắt" : "Bật";
                if (fanNextActionText) {
                    fanNextActionText.innerHTML = `Lịch trình tiếp theo: <strong>${nextAct} lúc ${nextTime}</strong>`;
                }
                if (fanIndicator) {
                    fanIndicator.classList.add("active");
                }
            } else {
                if (fanNextActionText) {
                    fanNextActionText.innerText = "Chưa kích hoạt lịch trình";
                }
                if (fanIndicator) {
                    fanIndicator.classList.remove("active");
                }
            }
        }
    });

    // ==========================================
    // Event Listeners for Dashboard Controls
    // ==========================================
    const inLightToggle = document.getElementById("indoor-light-toggle");
    if (inLightToggle) {
        inLightToggle.addEventListener("change", (e) => {
            set(ref(db, "devices/indoor_light/status"), e.target.checked);
        });
    }



    const inSchedEnable = document.getElementById("indoor-sched-enable");
    if (inSchedEnable) {
        inSchedEnable.addEventListener("change", (e) => {
            set(ref(db, "devices/indoor_light/schedule/enabled"), e.target.checked);
        });
    }

    const outLightToggle = document.getElementById("outdoor-light-toggle");
    if (outLightToggle) {
        outLightToggle.addEventListener("change", (e) => {
            set(ref(db, "devices/outdoor_light/status"), e.target.checked);
        });
    }

    const outModeAuto = document.getElementById("out-mode-auto");
    if (outModeAuto) {
        outModeAuto.addEventListener("click", () => {
            set(ref(db, "devices/outdoor_light/mode"), "auto");
        });
    }

    const outModeManual = document.getElementById("out-mode-manual");
    if (outModeManual) {
        outModeManual.addEventListener("click", () => {
            set(ref(db, "devices/outdoor_light/mode"), "manual");
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

    const roofModeAuto = document.getElementById("roof-mode-auto");
    if (roofModeAuto) {
        roofModeAuto.addEventListener("click", () => {
            set(ref(db, "devices/roof/mode"), "auto");
        });
    }

    const roofModeManual = document.getElementById("roof-mode-manual");
    if (roofModeManual) {
        roofModeManual.addEventListener("click", () => {
            set(ref(db, "devices/roof/mode"), "manual");
        });
    }

    const openRoofBtn = document.getElementById("btn-open-roof");
    if (openRoofBtn) {
        openRoofBtn.addEventListener("click", () => {
            set(ref(db, "devices/roof/status"), "open");
        });
    }

    const closeRoofBtn = document.getElementById("btn-close-roof");
    if (closeRoofBtn) {
        closeRoofBtn.addEventListener("click", () => {
            set(ref(db, "devices/roof/status"), "closed");
        });
    }

    // Điều khiển quạt từ công tắc trên giao diện
    const fanToggle = document.getElementById("fan-toggle");
    if (fanToggle) {
        fanToggle.addEventListener("change", (e) => {
            set(ref(db, "devices/fan/status"), e.target.checked);
        });
    }

    const fanSchedEnable = document.getElementById("fan-sched-enable");
    if (fanSchedEnable) {
        fanSchedEnable.addEventListener("change", (e) => {
            set(ref(db, "devices/fan/schedule/enabled"), e.target.checked);
        });
    }

    // Khởi tạo day pickers
    initDayPicker("indoor-day-picker", "devices/indoor_light/schedule/days", DAY_NAMES);
    initDayPicker("fan-day-picker", "devices/fan/schedule/days", DAY_NAMES);

    // Khởi tạo custom time pickers
    initPremiumTimePickers((pickerId, newVal) => {
        if (pickerId === "indoor-on-time") {
            set(ref(db, "devices/indoor_light/schedule/on_time"), newVal);
        } else if (pickerId === "indoor-off-time") {
            set(ref(db, "devices/indoor_light/schedule/off_time"), newVal);
        } else if (pickerId === "fan-on-time") {
            set(ref(db, "devices/fan/schedule/on_time"), newVal);
        } else if (pickerId === "fan-off-time") {
            set(ref(db, "devices/fan/schedule/off_time"), newVal);
        }
    });

    // Khởi tạo đồ thị
    initChart();
}
