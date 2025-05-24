// Attack Manager JavaScript
const attackForm = document.getElementById('attack-form');
const dynamicParams = document.getElementById('dynamic-params');
const attackStatus = document.getElementById('attack-status');
const startButton = document.getElementById('start-attack');
const stopButton = document.getElementById('stop-attack');

// Parameter templates for different attack types
const attackParams = {
    MOUSEJACKING: `
        <div class="form-group">
            <label for="payload-type">Payload Type:</label>
            <select id="payload-type" name="payloadType" required>
                <option value="0">Keyboard</option>
                <option value="1">Mouse</option>
                <option value="2">Multimedia</option>
            </select>
        </div>
        <div class="form-group">
            <label for="target-vid">Target VID (hex):</label>
            <input type="text" id="target-vid" name="targetVID" pattern="[0-9A-Fa-f]{4}" placeholder="0x0000">
        </div>
        <div class="form-group">
            <label for="target-pid">Target PID (hex):</label>
            <input type="text" id="target-pid" name="targetPID" pattern="[0-9A-Fa-f]{4}" placeholder="0x0000">
        </div>
    `,
    ROLLJAM: `
        <div class="form-group">
            <label for="record-time">Record Time (ms):</label>
            <input type="number" id="record-time" name="recordTime" min="100" max="10000" value="1000">
        </div>
        <div class="form-group">
            <label for="jam-time">Jam Time (ms):</label>
            <input type="number" id="jam-time" name="jamTime" min="100" max="10000" value="2000">
        </div>
        <div class="form-group">
            <label for="replay-count">Replay Count:</label>
            <input type="number" id="replay-count" name="replayCount" min="1" max="100" value="3">
        </div>
    `,
    BRUTEFORCE: `
        <div class="form-group">
            <label for="start-code">Start Code (hex):</label>
            <input type="text" id="start-code" name="startCode" pattern="[0-9A-Fa-f]+" required>
        </div>
        <div class="form-group">
            <label for="end-code">End Code (hex):</label>
            <input type="text" id="end-code" name="endCode" pattern="[0-9A-Fa-f]+" required>
        </div>
        <div class="form-group">
            <label for="code-length">Code Length (bits):</label>
            <input type="number" id="code-length" name="codeLength" min="4" max="64" value="12">
        </div>
    `,
    DIP_SWITCH: `
        <div class="form-group">
            <label for="switch-count">Number of Switches:</label>
            <input type="number" id="switch-count" name="switchCount" min="1" max="12" value="8">
        </div>
        <div class="form-group">
            <label for="start-state">Start State (binary):</label>
            <input type="text" id="start-state" name="startState" pattern="[01]+" placeholder="00000000">
        </div>
        <div class="form-group">
            <label for="end-state">End State (binary):</label>
            <input type="text" id="end-state" name="endState" pattern="[01]+" placeholder="11111111">
        </div>
    `,
    JAMMING: `
        <div class="form-group">
            <label for="jam-type">Jamming Type:</label>
            <select id="jam-type" name="jamType" required>
                <option value="0">Constant</option>
                <option value="1">Sweep</option>
                <option value="2">Random</option>
            </select>
        </div>
        <div class="form-group sweep-settings">
            <label for="sweep-start">Sweep Start (MHz):</label>
            <input type="number" id="sweep-start" name="sweepStart" step="0.01">
        </div>
        <div class="form-group sweep-settings">
            <label for="sweep-end">Sweep End (MHz):</label>
            <input type="number" id="sweep-end" name="sweepEnd" step="0.01">
        </div>
        <div class="form-group sweep-settings">
            <label for="sweep-steps">Sweep Steps:</label>
            <input type="number" id="sweep-steps" name="sweepSteps" min="2" max="100" value="10">
        </div>
    `
};

// Update form when attack type changes
document.getElementById('attack-type').addEventListener('change', (e) => {
    const attackType = e.target.value;
    dynamicParams.innerHTML = attackParams[attackType] || '';

    // Show/hide sweep settings based on jam type for jamming attacks
    if (attackType === 'JAMMING') {
        document.getElementById('jam-type').addEventListener('change', (e) => {
            const sweepSettings = document.querySelectorAll('.sweep-settings');
            sweepSettings.forEach(el => {
                el.style.display = e.target.value === '1' ? 'block' : 'none';
            });
        });
    }
});

// Handle form submission
attackForm.addEventListener('submit', async (e) => {
    e.preventDefault();
    const formData = new FormData(attackForm);
    const config = {
        type: formData.get('type'),
        frequency: parseFloat(formData.get('frequency')),
        modulation: parseInt(formData.get('modulation')),
        params: {}
    };

    // Build parameters based on attack type
    switch(config.type) {
        case 'MOUSEJACKING':
            config.params = {
                payloadType: parseInt(formData.get('payloadType')),
                targetVID: parseInt(formData.get('targetVID'), 16),
                targetPID: parseInt(formData.get('targetPID'), 16)
            };
            break;
        case 'ROLLJAM':
            config.params = {
                recordTime: parseInt(formData.get('recordTime')),
                jamTime: parseInt(formData.get('jamTime')),
                replayCount: parseInt(formData.get('replayCount'))
            };
            break;
        case 'BRUTEFORCE':
            config.params = {
                startCode: parseInt(formData.get('startCode'), 16),
                endCode: parseInt(formData.get('endCode'), 16),
                codeLength: parseInt(formData.get('codeLength'))
            };
            break;
        case 'DIP_SWITCH':
            config.params = {
                switchCount: parseInt(formData.get('switchCount')),
                startState: parseInt(formData.get('startState'), 2),
                endState: parseInt(formData.get('endState'), 2)
            };
            break;
        case 'JAMMING':
            config.params = {
                jamType: parseInt(formData.get('jamType'))
            };
            if (config.params.jamType === 1) {
                config.params.sweepStart = parseFloat(formData.get('sweepStart'));
                config.params.sweepEnd = parseFloat(formData.get('sweepEnd'));
                config.params.sweepSteps = parseInt(formData.get('sweepSteps'));
            }
            break;
    }

    try {
        const response = await fetch('/api/attack/start', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(config)
        });

        if (response.ok) {
            startButton.disabled = true;
            stopButton.disabled = false;
            attackStatus.classList.remove('hidden');
            startStatusMonitoring();
        } else {
            const error = await response.text();
            alert(`Failed to start attack: ${error}`);
        }
    } catch (err) {
        alert(`Error: ${err.message}`);
    }
});

// Handle stop button
stopButton.addEventListener('click', async () => {
    try {
        const response = await fetch('/api/attack/stop', {
            method: 'POST'
        });

        if (response.ok) {
            startButton.disabled = false;
            stopButton.disabled = true;
            stopStatusMonitoring();
        } else {
            const error = await response.text();
            alert(`Failed to stop attack: ${error}`);
        }
    } catch (err) {
        alert(`Error: ${err.message}`);
    }
});

// Status monitoring
let statusInterval;

function startStatusMonitoring() {
    statusInterval = setInterval(updateStatus, 1000);
}

function stopStatusMonitoring() {
    if (statusInterval) {
        clearInterval(statusInterval);
    }
}

async function updateStatus() {
    try {
        const response = await fetch('/api/attack/status');
        const status = await response.json();

        document.getElementById('current-status').textContent = status.running ? 'Running' : 'Stopped';
        document.getElementById('current-action').textContent = status.currentAction;
        
        const successRate = status.totalAttempts > 0 
            ? (status.successCount / status.totalAttempts * 100).toFixed(1)
            : 0;
        document.getElementById('success-rate').textContent = `${successRate}%`;
        
        document.getElementById('signal-strength').style.width = `${status.signalStrength}%`;

        if (status.totalAttempts > 0) {
            const progress = ((status.currentValue - status.startValue) / 
                            (status.endValue - status.startValue) * 100).toFixed(1);
            document.getElementById('attack-progress').style.width = `${progress}%`;
            document.getElementById('progress-text').textContent = `${progress}%`;
        }

        if (!status.running) {
            stopStatusMonitoring();
            startButton.disabled = false;
            stopButton.disabled = true;
        }
    } catch (err) {
        console.error('Failed to update status:', err);
    }
}
