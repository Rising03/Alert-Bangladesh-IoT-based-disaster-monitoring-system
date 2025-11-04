// ThingSpeak config
const CONFIG = {
    channelId: 3035795, 
    readApiKey: "5FOKR9WU2CNC3VWU", 
    updateInterval: 30000 // 30s
};

// Thresholds for risk evaluation
const THRESHOLDS = {
    temperature: { high: 30 },
    humidity: { high: 80, low: 30 },
    waterLevel: { medium: 50, high: 80 },
    soilMoisture: { low: 20 },
    pressure: { low: 1000 },
    rain: { high: 50 }
};

// Fetch latest data
async function fetchData() {
    try {
        const url = `https://api.thingspeak.com/channels/${CONFIG.channelId}/feeds.json?api_key=${CONFIG.readApiKey}&results=1`;
        const response = await fetch(url);
        const data = await response.json();

        if (data.feeds.length > 0) {
            const feed = data.feeds[0];

            // Update metrics
            updateMetric("temperature", feed.field1);
            updateMetric("humidity", feed.field2);
            updateMetric("waterLevel", feed.field6);
            updateMetric("soilMoisture", feed.field3);
            updateMetric("pressure", feed.field7);
            updateMetric("totalrain", feed.field6);

            // Update risks
            updateRisks(feed);

            // Last update
            document.getElementById("lastUpdate").textContent = 
                `Last updated: ${new Date(feed.created_at).toLocaleString()}`;
        }
    } catch (err) {
        console.error("Error fetching data:", err);
    }
}

function updateMetric(id, value) {
    let text = "--";

    if (value) {
        let num = parseFloat(value).toFixed(2);

        if (id == "temperature") {
            text = `${num} °C`;
        } else if (id == "humidity") {
            text = `${num} %`;
        } else if (id == "waterLevel") {
            text = `${num} cm`;
        } else if (id == "soilMoisture") {
            text = `${num} %`;
        } else if (id == "pressure") {
            text = `${num} hPa`;
        } else if (id == "totalrain") {
            text = `${num} mm`;
        } else {
            text = `${num}`;
        }
    }

    document.getElementById(id).textContent = text;
}


function updateRisks(feed) {
    const temp = parseFloat(feed.field1);
    const hum = parseFloat(feed.field2);
    const water = parseFloat(feed.field3);
    const soil = parseFloat(feed.field4);
    const pres = parseFloat(feed.field5);
    const rain = parseFloat(feed.field6);

    // Thunderstorm risk
    if (hum > THRESHOLDS.humidity.high && temp > THRESHOLDS.temperature.high && pres < THRESHOLDS.pressure.low) {
        setRisk("thunderstormLevel", "Critical", "thunderstormDesc", "Dangerous thunderstorm conditions detected.", "thunderSuggetion", "Stay indoors, unplug electronics, and avoid trees.");
    } else {
        setRisk("thunderstormLevel", "Low", "thunderstormDesc", "No major thunderstorm risk.", "thunderSuggetion", "Normal activities can continue.");
    }

    // Drought risk
    if (soil < THRESHOLDS.soilMoisture.low && hum < THRESHOLDS.humidity.low) {
        setRisk("droughtLevel", "High", "droughtDesc", "Soil moisture and humidity are very low, drought risk increasing.", "droughtSuggestion", "Implement water-saving practices, ration supplies, use drought-resistant crops.");
    }else{
        setRisk("droughtLevel", "Low", "droughtDesc", " Short dry spell, water supply stable.", "droughtSuggestion", "No major drought threat");
    }

    // Flood risk
    if (water > THRESHOLDS.waterLevel.high || rain > THRESHOLDS.rain.high) {
        setRisk("floodLevel", "High", "floodDesc", "Severe flooding risk detected due to water/rain levels.", "floodSuggestion", "Evacuate if advised, avoid driving through water, keep valuables safe, follow official warnings.");
    }else{
        setRisk("floodLevel", "Low", "floodDesc", "Minor waterlogging, low impact.", "floodSuggestion", "No major flood threate");   
    }

    // Cyclone risk
    if (pres < 990) {
        setRisk("cycloneLevel", "High", "cycloneDesc", "Very low pressure indicates cyclone risk!", "cycloneSuggestion", "Evacuate if instructed, stock emergency supplies, stay indoors, follow government advisories.");
    }else{
        setRisk("cycloneLevel", "Low", "cycloneDesc", " Weak storm, light rain, slight winds.", "cycloneSuggestion", "No major cyclone threat");

    }
}

function setRisk(levelId, level, descId, desc, suggestionId, suggestion) {
    const levelEl = document.getElementById(levelId);
    levelEl.textContent = level;
    levelEl.className = `risk-level level-${level.toLowerCase()}`;

    if (descId) document.getElementById(descId).textContent = desc;
    if (suggestionId && suggestion) document.getElementById(suggestionId).textContent = suggestion;
}

// Auto-update every 30s
setInterval(fetchData, CONFIG.updateInterval);
fetchData();
