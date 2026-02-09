# Home Assistant REST API Examples

This directory contains working examples demonstrating how to use libcurl to interact with the Home Assistant REST API.

## Examples

### ha_rest_curl_example.cpp

A comprehensive example showing:
- Testing connection to Home Assistant
- Getting single sensor states
- Getting all entity states
- Retrieving historical data
- Calling services (controlling devices)
- Proper error handling and HTTP status codes
- JSON response parsing (simplified)

## Building the Example

### Prerequisites

Install libcurl development library:

**Ubuntu/Debian:**
```bash
sudo apt-get install libcurl4-openssl-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install libcurl-devel
```

**macOS:**
```bash
brew install curl
```

### Compilation

**Using g++ directly:**
```bash
cd examples
g++ -o ha_rest_example ha_rest_curl_example.cpp -lcurl
```

**Using the Makefile:**
```bash
cd examples
make
```

## Running the Example

### Configuration

Set your Home Assistant URL and access token as environment variables:

```bash
export HA_URL="http://192.168.1.100:8123"
export HA_TOKEN="your_long_lived_access_token_here"
```

**How to create a long-lived access token:**
1. Log into Home Assistant web interface
2. Click your username in the bottom left
3. Scroll to "Long-Lived Access Tokens"
4. Click "Create Token"
5. Give it a name (e.g., "libcurl Example")
6. Copy the token (you won't see it again!)

### Run the Example

```bash
./ha_rest_example
```

### Example Output

```
=== Home Assistant REST API Example ===
Connecting to: http://192.168.1.100:8123

1. Testing connection...
   ✓ Connected successfully!

2. Getting temperature sensor state...
   Response: 
   Entity: sensor.living_room_temperature
   Name: Living Room Temperature
   State: 22.5 °C

3. Getting energy consumption sensor...
   Response: 
   Entity: sensor.energy_consumption
   Name: Energy Consumption
   State: 1250 W

...
```

## Security Notes

⚠️ **Important Security Considerations:**

1. **Never commit tokens to version control**
   - Use environment variables or secure configuration files
   - Add token files to `.gitignore`

2. **Use HTTPS in production**
   - Replace `http://` with `https://`
   - Ensure SSL certificate verification is enabled

3. **Protect configuration files**
   ```bash
   chmod 600 config.ini  # If storing token in a file
   ```

4. **Rotate tokens regularly**
   - Create new tokens periodically
   - Revoke old tokens when no longer needed

5. **Use restricted tokens**
   - Consider using tokens with limited permissions when available

## Next Steps

### For Production Use

1. **Use a proper JSON library**
   - [nlohmann/json](https://github.com/nlohmann/json) - Header-only C++ JSON library
   - Makes parsing responses much easier and safer

2. **Implement retry logic**
   - Handle network failures gracefully
   - Exponential backoff for retries

3. **Add connection pooling**
   - Reuse curl handles for multiple requests
   - Reduces connection overhead

4. **Implement caching**
   - Cache sensor states to reduce API calls
   - Use time-based expiration

5. **Use MQTT for real-time updates**
   - REST API is great for initial state and history
   - MQTT is better for real-time monitoring
   - See [../HA_MQTT_INTEGRATION.md](../HA_MQTT_INTEGRATION.md) for MQTT guide

### Additional Resources

- [HA_REST_API_GUIDE.md](../HA_REST_API_GUIDE.md) - Comprehensive REST API documentation
- [Home Assistant REST API Documentation](https://developers.home-assistant.io/docs/api/rest/)
- [libcurl Tutorial](https://curl.se/libcurl/c/libcurl-tutorial.html)
- [MQTT Integration Guide](../HA_MQTT_INTEGRATION.md) - For real-time updates

## Troubleshooting

### "Connection refused"
- Verify Home Assistant is running
- Check IP address and port (default: 8123)
- Ensure firewall allows connections

### "Authentication failed (401)"
- Check token is correct (no extra spaces)
- Token may have been revoked - create a new one
- Verify Authorization header format

### "Entity not found (404)"
- Verify entity_id exists in Home Assistant
- Check for typos in entity name
- Use `/api/states` to list all entities

### "curl: command not found" (compilation error)
- Install libcurl development libraries (see Prerequisites above)

### SSL certificate errors
- For testing with self-signed certificates:
  ```cpp
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);  // Insecure!
  ```
- For production, use proper SSL certificates

## License

This example code is provided for demonstration purposes as part of the home automation system project.
