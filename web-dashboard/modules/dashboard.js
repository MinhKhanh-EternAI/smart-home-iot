import { db, state } from "../app.js";
import { ref, onValue, set } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";

function getVietnameseDayOfWeek() {
    const days = ["Chủ Nhật", "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy"];
    const now = new Date();
    const dayName = days[now.getDay()];
    return `hôm nay (${dayName})`;
}

// Hàm kiểm tra cảnh báo thông minh: Trời mưa + Mái che mở ở chế độ thủ công
export function checkSmartAlerts() {
    const rainCard = document.getElementById("rain-status-card");
    const rainDetails = document.getElementById("rain-details");
    if (!rainCard || !rainDetails) return;
    
    if (state.sensors.rain && state.devices.roof.status === "open" && state.devices.roof.mode === "manual") {
        // Cảnh báo nguy hiểm khi trời mưa nhưng mở mái che thủ công
        rainCard.classList.add("warning-alarm");
        rainDetails.innerHTML = "⚠️ Nguy hiểm: Mái đang mở thủ công!";
    } else {
        rainCard.classList.remove("warning-alarm");
        
        // Cập nhật lại thông tin mái che bình thường
        const roof = state.devices.roof;
        rainDetails.innerText = `Mái che đang: ${roof.status === "open" ? "Mở" : "Đóng"} (${roof.mode === "auto" ? "Tự động" : "Thủ công"})`;
    }
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

const DAY_NAMES = ["sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"];

function updateDayPicker(pickerId, daysObj, disabled) {
    const picker = document.getElementById(pickerId);
    if (!picker) return;
    picker.querySelectorAll(".day-btn").forEach(btn => {
        const dayKey = DAY_NAMES[parseInt(btn.dataset.day)];
        btn.classList.toggle("active", daysObj && daysObj[dayKey] === true);
    });
    picker.classList.toggle("disabled", !!disabled);
}

function initDayPicker(pickerId, fbPath) {
    const picker = document.getElementById(pickerId);
    if (!picker) return;
    picker.addEventListener("click", (e) => {
        const btn = e.target.closest(".day-btn");
        if (!btn) return;
        const dayKey = DAY_NAMES[parseInt(btn.dataset.day)];
        btn.classList.toggle("active");
        set(ref(db, `${fbPath}/${dayKey}`), btn.classList.contains("active")).catch(() => {
            btn.classList.toggle("active");
        });
    });
}

export function initDashboard() {
    // Đọc giá trị Cảm biến
    const updateSensorsUI = (data) => {
        if (!data) return;

        // Cập nhật state toàn cục
        if (data.rain !== undefined) state.sensors.rain = !!data.rain;
        if (data.motion !== undefined) state.sensors.motion = !!data.motion;
        
        if (data.temperature !== undefined) state.sensors.temperature = data.temperature;
        else if (data.temp !== undefined) state.sensors.temperature = data.temp;
        
        if (data.humidity !== undefined) state.sensors.humidity = data.humidity;

        // Cập nhật Nhiệt độ / Độ ẩm
        const tempEl = document.getElementById("temp-val");
        const humEl = document.getElementById("hum-val");
        if (tempEl) tempEl.innerText = (state.sensors.temperature !== null && state.sensors.temperature !== undefined) ? parseFloat(state.sensors.temperature).toFixed(1) : "--";
        if (humEl) humEl.innerText = (state.sensors.humidity !== null && state.sensors.humidity !== undefined) ? parseFloat(state.sensors.humidity).toFixed(1) : "--";

        // Trạng thái mưa
        const rainCard = document.getElementById("rain-status-card");
        const rainVal = document.getElementById("rain-val");
        if (rainCard && rainVal && data.rain !== undefined) {
            if (state.sensors.rain) {
                rainCard.classList.add("raining");
                rainVal.innerText = "Trời Đang Mưa";
            } else {
                rainCard.classList.remove("raining");
                rainVal.innerText = "Không Mưa";
            }
        }

        // Trạng thái chuyển động
        const motionCard = document.getElementById("motion-status-card");
        const motionVal = document.getElementById("motion-val");
        if (motionCard && motionVal && data.motion !== undefined) {
            if (state.sensors.motion) {
                motionCard.classList.add("detected");
                motionVal.innerText = "Có Chuyển Động";
            } else {
                motionCard.classList.remove("detected");
                motionVal.innerText = "Yên Tĩnh";
            }
        }

        // Kiểm tra cảnh báo thông minh
        checkSmartAlerts();
    };

    onValue(ref(db, "sensors"), (snapshot) => {
        updateSensorsUI(snapshot.val());
    });

    // Đọc và đồng bộ các Thiết bị (Devices)
    onValue(ref(db, "devices"), (snapshot) => {
        const data = snapshot.val();
        if (!data) return;

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

        // Chế độ Đèn Trong Nhà (Manual/Schedule)
        const inModeManual = document.getElementById("in-mode-manual");
        const inModeSchedule = document.getElementById("in-mode-schedule");
        const inScheduleSection = document.getElementById("indoor-schedule-section");
        const isIndoorSched = (inLight.mode === "schedule");
        if (inScheduleSection) {
            if (isIndoorSched) {
                if (inModeSchedule) inModeSchedule.classList.add("active");
                if (inModeManual) inModeManual.classList.remove("active");
                inScheduleSection.style.display = "block";
            } else {
                if (inModeSchedule) inModeSchedule.classList.remove("active");
                if (inModeManual) inModeManual.classList.add("active");
                inScheduleSection.style.display = "none";
            }
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
            updateDayPicker("indoor-day-picker", inDays, !inLight.schedule.enabled);

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
        const outLight = data.outdoor_light || { status: false, mode: "auto", schedule: { enabled: false, on_time: "18:00", off_time: "06:00" } };
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
        const outScheduleSection = document.getElementById("outdoor-schedule-section");
        const outAutoInfo = document.getElementById("outdoor-auto-info");
        const isAuto = (outLight.mode === "auto");
        if (isAuto) {
            if (outModeAuto) outModeAuto.classList.add("active");
            if (outModeManual) outModeManual.classList.remove("active");
            if (outScheduleSection) outScheduleSection.style.display = "none";
            if (outAutoInfo) outAutoInfo.style.display = "flex";
        } else {
            if (outModeAuto) outModeAuto.classList.remove("active");
            if (outModeManual) outModeManual.classList.add("active");
            if (outScheduleSection) outScheduleSection.style.display = "block";
            if (outAutoInfo) outAutoInfo.style.display = "none";
        }
        
        // Hẹn giờ Đèn ngoài sân
        if (outLight.schedule) {
            const enableCheck = document.getElementById("outdoor-sched-enable");
            if (enableCheck) enableCheck.checked = outLight.schedule.enabled;
            
            // Cập nhật bộ chọn giờ mới
            if (window.updatePicker_outdoor_on_time) window.updatePicker_outdoor_on_time(outLight.schedule.on_time || "18:00");
            if (window.updatePicker_outdoor_off_time) window.updatePicker_outdoor_off_time(outLight.schedule.off_time || "06:00");

            // UX: Vô hiệu hóa ô chọn giờ và day picker nếu không bật hẹn giờ
            if (outScheduleSection) {
                const outdoorSchedPickers = outScheduleSection.querySelector(".schedule-picker-grid-premium");
                if (outdoorSchedPickers) {
                    outdoorSchedPickers.classList.toggle("disabled", !outLight.schedule.enabled);
                }
            }
            const outDays = outLight.schedule.days || { monday: true, tuesday: true, wednesday: true, thursday: true, friday: true, saturday: true, sunday: true };
            updateDayPicker("outdoor-day-picker", outDays, !outLight.schedule.enabled);

            // Hiển thị trạng thái lịch trình tiếp theo
            const outdoorNextActionText = document.getElementById("outdoor-next-action-text");
            const outdoorIndicator = document.getElementById("outdoor-indicator-dot");
            if (outScheduleSection) {
                outScheduleSection.classList.toggle("active", outLight.schedule.enabled);
            }
            if (outLight.schedule.enabled) {
                const nextTime = outLight.status ? (outLight.schedule.off_time || "06:00") : (outLight.schedule.on_time || "18:00");
                const nextAct = outLight.status ? "Tắt" : "Bật";
                if (outdoorNextActionText) {
                    outdoorNextActionText.innerHTML = `Lịch trình tiếp theo: <strong>${nextAct} lúc ${nextTime}</strong>`;
                }
                if (outdoorIndicator) {
                    outdoorIndicator.classList.add("active");
                }
            } else {
                if (outdoorNextActionText) {
                    outdoorNextActionText.innerText = "Chưa kích hoạt lịch trình";
                }
                if (outdoorIndicator) {
                    outdoorIndicator.classList.remove("active");
                }
            }
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

        // Kiểm tra cảnh báo thông minh
        checkSmartAlerts();
    });

    // Đọc trạng thái Quạt từ Firebase
    onValue(ref(db, "devices/fan"), (snapshot) => {
        const fanOn = !!snapshot.val();
        const toggle = document.getElementById("fan-toggle");
        const card = document.getElementById("fan-card-container");
        const statusText = document.getElementById("fan-status-text");
        const icon = document.getElementById("fan-icon");

        if (toggle) toggle.checked = fanOn;
        if (card) card.classList.toggle("active", fanOn);
        if (statusText) {
            statusText.innerText = fanOn ? "Đang Bật" : "Đang Tắt";
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

    const inModeManual = document.getElementById("in-mode-manual");
    if (inModeManual) {
        inModeManual.addEventListener("click", () => {
            set(ref(db, "devices/indoor_light/mode"), "manual");
        });
    }

    const inModeSchedule = document.getElementById("in-mode-schedule");
    if (inModeSchedule) {
        inModeSchedule.addEventListener("click", () => {
            set(ref(db, "devices/indoor_light/mode"), "schedule");
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

    const outSchedEnable = document.getElementById("outdoor-sched-enable");
    if (outSchedEnable) {
        outSchedEnable.addEventListener("change", (e) => {
            set(ref(db, "devices/outdoor_light/schedule/enabled"), e.target.checked);
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
            set(ref(db, "devices/fan"), e.target.checked);
        });
    }

    // Khởi tạo day pickers
    initDayPicker("indoor-day-picker", "devices/indoor_light/schedule/days");
    initDayPicker("outdoor-day-picker", "devices/outdoor_light/schedule/days");

    // Khởi tạo custom time pickers
    initPremiumTimePickers((pickerId, newVal) => {
        if (pickerId === "indoor-on-time") {
            set(ref(db, "devices/indoor_light/schedule/on_time"), newVal);
        } else if (pickerId === "indoor-off-time") {
            set(ref(db, "devices/indoor_light/schedule/off_time"), newVal);
        } else if (pickerId === "outdoor-on-time") {
            set(ref(db, "devices/outdoor_light/schedule/on_time"), newVal);
        } else if (pickerId === "outdoor-off-time") {
            set(ref(db, "devices/outdoor_light/schedule/off_time"), newVal);
        }
    });
}
