// Track navigation state and abort controller globally
let isNavigating = false;
let abortControllers = [];
let navigationTimeout = null;
const isHomePage = window.location.pathname === '/' || window.location.pathname === '/index.html';
let lastConnectionCheck = 0;

function checkConnection() {
    const now = Date.now();
    if (now - lastConnectionCheck < 2000) return;
    lastConnectionCheck = now;

    if (isNavigating) {
        abortAllRequests();
        return;
    }

    const controller = new AbortController();
    abortControllers.push(controller);

    const timeout = setTimeout(() => controller.abort(), 1500);

    fetch(isHomePage ? '/stats' : '/connectioncheck', { 
        signal: controller.signal 
    })
    .then(response => {
        clearTimeout(timeout);
        if (isNavigating) return;
        if (isHomePage) {
            return response.json().then(data => {
                updateStats(data);
            });
        } else {
            updateConnectionStatus(response.ok);
            return response.json();
        }
    })
    .catch(error => {
        clearTimeout(timeout);
        if (error.name !== 'AbortError' && !isNavigating) {
            updateConnectionStatus(false);
            if (isHomePage) {
                document.getElementById('uptime').innerText = 'N/A';
                document.getElementById('cpu0').innerText = 'N/A';
                document.getElementById('cpu1').innerText = 'N/A';
                document.getElementById('temperature').innerText = 'N/A';
                document.getElementById('freespiffs').innerText = 'N/A';
                document.getElementById('totalram').innerText = 'N/A';
                document.getElementById('freeram').innerText = 'N/A';
                document.getElementById('ssid').innerText = 'N/A';
                document.getElementById('ipaddress').innerText = 'N/A';
                document.getElementById('sdcard_present').innerText = 'No';
                document.getElementById('sdcard_size_gb').innerText = 'N/A';
                document.getElementById('sdcard_free_gb').innerText = 'N/A';
            }
        }
    });
}

function abortAllRequests() {
    abortControllers.forEach(controller => controller.abort());
    abortControllers = [];
}

function setupNavigation() {
    const links = document.querySelectorAll("#menu a");

    links.forEach(link => {
        link.addEventListener('click', function(e) {
            if (this.classList.contains('active')) {
                e.preventDefault();
                return;
            }

            isNavigating = true;
            abortAllRequests();
            document.body.classList.add('page-loading');

            if (/iPad|iPhone|iPod/.test(navigator.userAgent)) {
                e.preventDefault();
                setTimeout(() => {
                    window.location.replace(this.href);
                }, 50);
            } else {
                setTimeout(() => {
                    window.location.href = this.href;
                }, 100);
            }
        });
    });

    const currentPath = window.location.pathname;
    links.forEach(link => {
        if (link.getAttribute('href') === currentPath) {
            link.classList.add('active');
        } else {
            link.classList.remove('active');
        }
    });
}

window.addEventListener('load', () => {
    isNavigating = false;
    document.body.classList.remove('page-loading');
});

function updateConnectionStatus(isOnline) {
    document.querySelectorAll('.status-indicator').forEach(indicator => {
        indicator.classList.toggle('status-online', isOnline);
        indicator.classList.toggle('status-offline', !isOnline);
    });

    if (window.location.pathname !== '/') {
        document.querySelectorAll('.rf-logo').forEach(title => {
            title.classList.toggle('online', isOnline);
            title.classList.toggle('offline', !isOnline);
        });
    }
}

function updateStats(data) {
    if (data.uptime) document.getElementById('uptime').innerText = formatUptime(data.uptime);
    if (data.cpu0) document.getElementById('cpu0').innerText = data.cpu0 + ' MHz';
    if (data.cpu1) document.getElementById('cpu1').innerText = data.cpu1 + ' MHz';
    if (data.temperature) document.getElementById('temperature').innerText = data.temperature.toFixed(1) + ' °C';
    if (data.freespiffs) document.getElementById('freespiffs').innerText = formatBytes(data.freespiffs);
    if (data.totalram) document.getElementById('totalram').innerText = formatBytes(data.totalram);
    if (data.freeram) document.getElementById('freeram').innerText = formatBytes(data.freeram);
    if (data.ssid) document.getElementById('ssid').innerText = data.ssid;
    if (data.ipaddress) document.getElementById('ipaddress').innerText = data.ipaddress;
    if (data.sdcard_present !== undefined) {
        document.getElementById('sdcard_present').innerText = data.sdcard_present ? 'Yes' : 'No';
    }
    if (data.sdcard_size_gb) document.getElementById('sdcard_size_gb').innerText = data.sdcard_size_gb + ' GB';
    if (data.sdcard_free_gb) document.getElementById('sdcard_free_gb').innerText = data.sdcard_free_gb + ' GB';

    updateConnectionStatus(true);
}

function formatBytes(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;
    
    if (days > 0) return `${days}d ${hours}h ${minutes}m ${secs}s`;
    else if (hours > 0) return `${hours}h ${minutes}m ${secs}s`;
    else if (minutes > 0) return `${minutes}m ${secs}s`;
    else return `${secs}s`;
}

function showMessage(type, text) {
    const container = document.getElementById('global-toast') || document.createElement('div');
    if (!container.id) {
        container.id = 'global-toast';
        container.className = 'toast-container';
        document.body.appendChild(container);
    }
    
    const toast = document.createElement('div');
    toast.className = `toast-message ${type}`;
    
    const messageSpan = document.createElement('span');
    messageSpan.textContent = text;
    
    const closeButton = document.createElement('span');
    closeButton.className = 'toast-close';
    closeButton.innerHTML = '&times;';
    closeButton.onclick = () => {
        toast.style.animation = 'toastFadeOut 0.3s ease-out';
        setTimeout(() => toast.remove(), 300);
    };
    
    toast.appendChild(messageSpan);
    toast.appendChild(closeButton);
    container.appendChild(toast);
    
    const timer = setTimeout(() => {
        toast.style.animation = 'toastFadeOut 0.3s ease-out';
        setTimeout(() => toast.remove(), 300);
    }, 5000);
    
    closeButton.onclick = () => {
        clearTimeout(timer);
        toast.style.animation = 'toastFadeOut 0.3s ease-out';
        setTimeout(() => toast.remove(), 300);
    };
}

document.addEventListener('touchstart', function(event) {
    if (event.touches.length > 1) event.preventDefault();
}, { passive: false });

document.addEventListener('gesturestart', function(e) {
    e.preventDefault();
});

document.addEventListener("DOMContentLoaded", function () {
    console.log('EvilCrow RF - Initializing...');
    setupNavigation();
    isNavigating = false;
    document.body.classList.remove('page-loading');
    setInterval(checkConnection, 5000);
    checkConnection();
    console.log('EvilCrow RF - Initialization complete');
});

if (document.readyState === 'complete' || document.readyState === 'interactive') {
    setTimeout(() => {
        if (typeof setupNavigation === 'function') setupNavigation();
        if (typeof checkConnection === 'function') {
            setInterval(checkConnection, 5000);
            checkConnection();
        }
    }, 100);
}

