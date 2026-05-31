let refreshInterval;
let devices = [];
let recentData = [];
let chartData = { temperature: [], humidity: [] };

document.addEventListener('DOMContentLoaded', function() {
    setupNavigation();
    initializeApp();
    
    refreshInterval = setInterval(function() {
        loadLatestData();
        updateDeviceStatus();
    }, 3000);
});

function setupNavigation() {
    const navLinks = document.querySelectorAll('nav a');
    
    navLinks.forEach(link => {
        link.addEventListener('click', function(e) {
            e.preventDefault();
            
            navLinks.forEach(l => l.classList.remove('active'));
            this.classList.add('active');
            
            const sectionId = this.getAttribute('href').substring(1);
            const sections = document.querySelectorAll('main section');
            
            sections.forEach(section => {
                section.classList.remove('active');
                if (section.id === sectionId) {
                    section.classList.add('active');
                }
            });
            
            if (sectionId === 'devices') {
                loadDevices();
            } else if (sectionId === 'history') {
                loadHistory();
            }
        });
    });
}

function initializeApp() {
    loadLatestData();
    loadDevices();
    loadHistory();
    updateMQTTStatus();
}

function updateMQTTStatus() {
    fetch('/cgi-bin/sensor_data.cgi?action=status')
        .then(response => response.json())
        .then(data => {
            const statusDot = document.getElementById('mqtt-status');
            const statusText = document.getElementById('mqtt-status-text');
            
            if (data.connected) {
                statusDot.className = 'status-dot online';
                statusText.textContent = 'MQTT已连接';
            } else {
                statusDot.className = 'status-dot';
                statusText.textContent = 'MQTT未连接';
            }
        })
        .catch(() => {
            document.getElementById('mqtt-status').className = 'status-dot';
            document.getElementById('mqtt-status-text').textContent = '未知';
        });
}

function loadLatestData() {
    fetch('/cgi-bin/sensor_data.cgi?action=latest')
        .then(response => response.json())
        .then(data => {
            if (data.error) {
                console.error('Error:', data.error);
                return;
            }
            
            updateDashboard(data);
            addToRecentData(data);
            updateCharts(data);
        })
        .catch(error => console.error('Fetch error:', error));
}

function updateDashboard(data) {
    document.getElementById('last-update').textContent = 
        data.timestamp ? new Date(data.timestamp * 1000).toLocaleString('zh-CN') : '--';
    
    if (data.data) {
        const deviceName = data.data.device || data.data.sourceDevice || '未知设备';
        
        if (!devices.find(d => d.name === deviceName)) {
            addNewDevice(deviceName);
        }
        
        const device = devices.find(d => d.name === deviceName);
        if (device) {
            device.lastSeen = Date.now();
            device.online = true;
            device.lastData = data.data;
        }
        
        updateOverviewCards();
    }
}

function addToRecentData(data) {
    if (!data.data || !data.timestamp) return;
    
    const deviceName = data.data.device || data.data.sourceDevice || '未知设备';
    const newItem = {
        timestamp: data.timestamp,
        device: deviceName,
        temperature: data.data.temperature,
        humidity: data.data.humidity
    };
    
    if (recentData.length > 0) {
        const latest = recentData[0];
        if (latest.timestamp === newItem.timestamp && 
            latest.device === newItem.device &&
            latest.temperature === newItem.temperature &&
            latest.humidity === newItem.humidity) {
            return;
        }
    }
    
    recentData.unshift(newItem);
    if (recentData.length > 10) {
        recentData.pop();
    }
    
    updateRecentDataList();
}

function updateRecentDataList() {
    const list = document.getElementById('recent-data-list');
    
    if (recentData.length === 0) {
        list.innerHTML = '<div class="data-item loading">等待数据...</div>';
        return;
    }
    
    let html = '';
    recentData.forEach(item => {
        const time = new Date(item.timestamp * 1000).toLocaleTimeString('zh-CN');
        const deviceClass = item.device.toLowerCase().includes('stm32') ? 'device-stm32' : 
                           (item.device.toLowerCase().includes('ch32') ? 'device-ch32' : '');
        
        html += `
            <div class="data-item ${deviceClass}">
                <span class="data-device">${item.device}</span>
                <span class="data-values">
                    <span>🌡️ ${item.temperature ? parseFloat(item.temperature).toFixed(1) + '°C' : '--'}</span>
                    <span>💧 ${item.humidity ? parseFloat(item.humidity).toFixed(1) + '%' : '--'}</span>
                </span>
                <span class="data-time">${time}</span>
            </div>
        `;
    });
    
    list.innerHTML = html;
}

function updateCharts(data) {
    if (!data.data) return;
    
    const temp = parseFloat(data.data.temperature);
    const hum = parseFloat(data.data.humidity);
    
    if (!isNaN(temp)) {
        chartData.temperature.push({ time: Date.now(), value: temp });
        if (chartData.temperature.length > 20) chartData.temperature.shift();
    }
    if (!isNaN(hum)) {
        chartData.humidity.push({ time: Date.now(), value: hum });
        if (chartData.humidity.length > 20) chartData.humidity.shift();
    }
    
    drawChart();
}

function drawChart() {
    const canvas = document.getElementById('data-chart');
    if (!canvas) return;
    
    const ctx = canvas.getContext('2d');
    const width = canvas.width;
    const height = canvas.height;
    
    ctx.clearRect(0, 0, width, height);
    
    const chartType = document.getElementById('chart-type').value;
    const data = chartData[chartType];
    
    if (data.length < 2) return;
    
    const padding = 30;
    const chartWidth = width - padding * 2;
    const chartHeight = height - padding * 2;
    
    const values = data.map(d => d.value);
    const minVal = Math.min(...values) - 5;
    const maxVal = Math.max(...values) + 5;
    
    ctx.strokeStyle = '#667eea';
    ctx.lineWidth = 2;
    ctx.beginPath();
    
    data.forEach((point, index) => {
        const x = padding + (index / (data.length - 1)) * chartWidth;
        const y = padding + chartHeight - ((point.value - minVal) / (maxVal - minVal)) * chartHeight;
        
        if (index === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
    });
    ctx.stroke();
    
    ctx.fillStyle = 'rgba(102, 126, 234, 0.1)';
    ctx.lineTo(padding + chartWidth, padding + chartHeight);
    ctx.lineTo(padding, padding + chartHeight);
    ctx.closePath();
    ctx.fill();
    
    const lastPoint = data[data.length - 1];
    const lastX = padding + chartWidth;
    const lastY = padding + chartHeight - ((lastPoint.value - minVal) / (maxVal - minVal)) * chartHeight;
    
    ctx.beginPath();
    ctx.arc(lastX, lastY, 5, 0, Math.PI * 2);
    ctx.fillStyle = '#667eea';
    ctx.fill();
    
    ctx.fillStyle = '#666';
    ctx.font = '10px Arial';
    ctx.fillText(lastPoint.value.toFixed(1), lastX + 8, lastY + 4);
}

function updateChart() {
    drawChart();
}

function addNewDevice(deviceName) {
    devices.push({
        name: deviceName,
        online: true,
        lastSeen: Date.now(),
        lastData: null,
        firstSeen: Date.now()
    });
    
    updateDeviceSelects();
    loadDevices();
}

function updateDeviceSelects() {
    const chartDevice = document.getElementById('chart-device');
    const historyDevice = document.getElementById('history-device');
    
    const deviceNames = devices.map(d => d.name);
    const existingChartOptions = Array.from(chartDevice.options).map(o => o.value);
    const existingHistoryOptions = Array.from(historyDevice.options).map(o => o.value);
    
    deviceNames.forEach(name => {
        if (!existingChartOptions.includes(name)) {
            const option = document.createElement('option');
            option.value = name;
            option.textContent = name;
            chartDevice.appendChild(option);
        }
        if (!existingHistoryOptions.includes(name)) {
            const option = document.createElement('option');
            option.value = name;
            option.textContent = name;
            historyDevice.appendChild(option);
        }
    });
}

function updateDeviceStatus() {
    const offlineTimeout = 30000;
    
    devices.forEach(device => {
        if (device.online && Date.now() - device.lastSeen > offlineTimeout) {
            device.online = false;
        }
    });
    
    updateOverviewCards();
}

function updateOverviewCards() {
    document.getElementById('device-count').textContent = devices.length;
    
    const onlineCount = devices.filter(d => d.online).length;
    document.getElementById('online-count').textContent = onlineCount;
}

function loadDevices() {
    fetch('/cgi-bin/device_mgmt.cgi')
        .then(response => response.json())
        .then(data => {
            if (data.devices && data.devices.length > 0) {
                data.devices.forEach(d => {
                    if (!devices.find(existing => existing.name === d.device_id)) {
                        devices.push({
                            name: d.device_id,
                            online: true,
                            lastSeen: Date.now(),
                            lastData: null,
                            firstSeen: Date.now(),
                            ip: d.ip
                        });
                    }
                });
            }
            
            updateDeviceSelects();
            renderDeviceGrid();
        })
        .catch(() => renderDeviceGrid());
}

function renderDeviceGrid() {
    const grid = document.getElementById('device-grid');
    
    if (devices.length === 0) {
        grid.innerHTML = '<div class="device-card loading">暂无设备</div>';
        return;
    }
    
    const filter = document.getElementById('device-filter').value;
    let filteredDevices = devices;
    
    if (filter === 'online') {
        filteredDevices = devices.filter(d => d.online);
    } else if (filter === 'offline') {
        filteredDevices = devices.filter(d => !d.online);
    }
    
    if (filteredDevices.length === 0) {
        grid.innerHTML = '<div class="device-card loading">没有符合条件的设备</div>';
        return;
    }
    
    let html = '';
    filteredDevices.forEach(device => {
        const onlineClass = device.online ? 'online' : 'offline';
        const lastSeen = device.lastSeen ? 
            new Date(device.lastSeen).toLocaleString('zh-CN') : '未知';
        const firstSeen = device.firstSeen ? 
            new Date(device.firstSeen).toLocaleString('zh-CN') : '未知';
        
        let latestHtml = '';
        if (device.lastData) {
            latestHtml = `
                <div class="device-latest">
                    <div class="latest-label">最新数据</div>
                    <div class="latest-values">
                        <span>🌡️ ${device.lastData.temperature ? parseFloat(device.lastData.temperature).toFixed(1) + '°C' : '--'}</span>
                        <span>💧 ${device.lastData.humidity ? parseFloat(device.lastData.humidity).toFixed(1) + '%' : '--'}</span>
                    </div>
                </div>
            `;
        }
        
        html += `
            <div class="device-card ${onlineClass}">
                <h3>📱 ${device.name}</h3>
                <div class="device-status">
                    <span class="status-badge ${onlineClass}">${device.online ? '在线' : '离线'}</span>
                </div>
                <div class="device-info">
                    <div>
                        <span class="info-label">首次发现:</span>
                        <span class="info-value">${firstSeen}</span>
                    </div>
                    <div>
                        <span class="info-label">最后活跃:</span>
                        <span class="info-value">${lastSeen}</span>
                    </div>
                    ${device.ip ? `
                    <div>
                        <span class="info-label">IP地址:</span>
                        <span class="info-value">${device.ip}</span>
                    </div>
                    ` : ''}
                </div>
                ${latestHtml}
            </div>
        `;
    });
    
    grid.innerHTML = html;
}

function filterDevices() {
    renderDeviceGrid();
}

function refreshDevices() {
    loadDevices();
}

function loadHistory() {
    const device = document.getElementById('history-device').value;
    const period = document.getElementById('history-period').value;
    
    fetch(`/cgi-bin/sensor_data.cgi?action=history&device=${device}&period=${period}`)
        .then(response => response.json())
        .then(data => {
            renderHistoryTable(data);
            calculateStats(data);
        })
        .catch(error => {
            console.error('Fetch error:', error);
            document.getElementById('history-table-body').innerHTML = 
                '<tr><td colspan="5">无法获取历史数据</td></tr>';
        });
}

function renderHistoryTable(data) {
    const tbody = document.getElementById('history-table-body');
    
    if (!data.data || data.data.length === 0) {
        tbody.innerHTML = '<tr><td colspan="5">暂无历史数据</td></tr>';
        return;
    }
    
    let html = '';
    data.data.forEach(item => {
        const time = item.timestamp ? 
            new Date(item.timestamp * 1000).toLocaleString('zh-CN') : '未知';
        const device = item.device || item.data?.device || item.data?.sourceDevice || '未知';
        const temp = item.data?.temperature !== undefined ? 
            parseFloat(item.data.temperature).toFixed(1) : '--';
        const hum = item.data?.humidity !== undefined ? 
            parseFloat(item.data.humidity).toFixed(1) : '--';
        
        html += `
            <tr>
                <td>${time}</td>
                <td>${device}</td>
                <td>${temp}</td>
                <td>${hum}</td>
                <td>MQTT</td>
            </tr>
        `;
    });
    
    tbody.innerHTML = html;
}

function calculateStats(data) {
    if (!data.data || data.data.length === 0) {
        document.getElementById('temp-max').textContent = '--';
        document.getElementById('temp-min').textContent = '--';
        document.getElementById('temp-avg').textContent = '--';
        document.getElementById('hum-max').textContent = '--';
        document.getElementById('hum-min').textContent = '--';
        document.getElementById('hum-avg').textContent = '--';
        return;
    }
    
    const temps = [];
    const hums = [];
    
    data.data.forEach(item => {
        if (item.data?.temperature !== undefined) {
            temps.push(parseFloat(item.data.temperature));
        }
        if (item.data?.humidity !== undefined) {
            hums.push(parseFloat(item.data.humidity));
        }
    });
    
    if (temps.length > 0) {
        document.getElementById('temp-max').textContent = Math.max(...temps).toFixed(1);
        document.getElementById('temp-min').textContent = Math.min(...temps).toFixed(1);
        document.getElementById('temp-avg').textContent = 
            (temps.reduce((a, b) => a + b, 0) / temps.length).toFixed(1);
    } else {
        document.getElementById('temp-max').textContent = '--';
        document.getElementById('temp-min').textContent = '--';
        document.getElementById('temp-avg').textContent = '--';
    }
    
    if (hums.length > 0) {
        document.getElementById('hum-max').textContent = Math.max(...hums).toFixed(1);
        document.getElementById('hum-min').textContent = Math.min(...hums).toFixed(1);
        document.getElementById('hum-avg').textContent = 
            (hums.reduce((a, b) => a + b, 0) / hums.length).toFixed(1);
    } else {
        document.getElementById('hum-max').textContent = '--';
        document.getElementById('hum-min').textContent = '--';
        document.getElementById('hum-avg').textContent = '--';
    }
    
    document.getElementById('data-count').textContent = data.data.length;
}

window.addEventListener('beforeunload', function() {
    if (refreshInterval) {
        clearInterval(refreshInterval);
    }
});
