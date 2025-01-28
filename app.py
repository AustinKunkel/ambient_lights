from quart import Quart, jsonify, request, render_template
from quart_cors import cors
import asyncio
import json

# import led_functions
import led_functions as lf
import screen_capture as sc
import sound_capture


app = Quart(__name__, static_folder='static')
cors(app)  # Enable CORS

@app.route('/')
async def index():
    return await render_template('index.html')

@app.route('/led', methods=['GET'])
async def handle_get_led():
    try:
        global current_task

        # Extract the query string
        params = request.args

        if(len(params) == 0):
            return jsonify(lf.led_values)
        
        flag = change_dict_vars(params=params, d=lf.led_values, collision=["bri"])

        write_json_data("json/led_values.json", lf.led_values)

        # Execute the LED update asynchronously
        await lf.update_led_vars() if flag else await lf.update_leds()


        return jsonify(lf.led_values)
    except Exception as e:
        print(f"Error: {e}")  # Log the exception
        return jsonify({"error": "An internal server error occurred"}), 500

@app.route('/scrncapt', methods=['GET'])
async def handle_get_scrn_capt():
    try:
        global current_task

        params = request.args

        change_dict_vars(params=params, d=sc.sc_settings)

        write_json_data("json/capt_values.json", sc.sc_settings)

        await lf.update_leds()
        print("updated leds")

        return jsonify(sc.sc_settings)

    except Exception as e:
        print(f"Error: {e}")  # Log the exception
        return jsonify({"error": "An internal server error occurred"}), 500
    
def change_dict_vars(params, d, collision = []):
    """
    Helper function for when changing dictonary variables
    will go through params and add to dict if key in dict.

    Collision is there to see if a specified key from params
    is in the dictionary

    will return true if there is a collision in the array
    """
    flag = False
    for key, value in params.items():
        if key in d:
            d[key] = value
            flag = key in collision
    return flag


@app.route('/led/status', methods=['GET'])
async def handle_get_status():
    """Handle GET request to retrieve LED status."""
    return jsonify(lf.led_values)

@app.route('/led/col', methods=['GET'])
async def handle_get_color():
    """Handles GET color"""
    return jsonify(lf.led_values['col'])

@app.route('/led/user/colors', methods=['GET'])
async def handle_get_user_colors():
    """Handles GET user colors"""
    file_path = "json/user_colors.json"

    try:
        colors = get_json_data(file_path)
        return jsonify(colors)
    except FileNotFoundError:
        return jsonify({"error": "File not found"}), 404
    except json.JSONDecodeError:
        return jsonify({"error": "Error decoding JSON"}), 500

@app.route('/led/user/colors/add', methods=['POST'])
async def handle_add_user_color():
    """Handles adding a user color"""
    data = await request.get_json()  # Get the JSON data from the request
    color = data.get('color')  # Extract the color
    file_path = "json/user_colors.json"

    if not color:
        return jsonify({"error": "No color provided"}), 400

    try:
        orig_colors = get_json_data(file_path)
    except FileNotFoundError:
        return jsonify({"error": "File not found"}), 404
    except json.JSONDecodeError:
        return jsonify({"error": "Error decoding JSON"}), 500

    orig_colors["colors"].append(color)

    try:
        write_json_data(file_path, orig_colors)
    except FileNotFoundError:
        return jsonify({"error": "File not found"}), 404
    except Exception as e:
        return jsonify({"error": str(e)}), 500

    # Assuming we add the color successfully
    response = {"message": "Color added successfully", "colors": orig_colors['colors']}
    return jsonify(response), 200

@app.route('/led/user/colors/remove', methods=['POST'])
async def handle_remove_user_color():
    data = await request.get_json()  # Get the JSON data from the request
    color = data.get('color')  # Extract the color
    if not color:
        return jsonify({"error": "No color provided"}), 400

    file_path = "json/user_colors.json"

    try:
        orig_colors = get_json_data(file_path)
    except FileNotFoundError:
        return jsonify({"error": "File not found"}), 404
    except json.JSONDecodeError:
        return jsonify({"error": "Error decoding JSON"}), 500
    
    colors =  orig_colors["colors"]
    if color in colors:
        colors.remove(color)
    else:
        return jsonify({"error": "Color not found"}), 404
    
    try:
        write_json_data(file_path, orig_colors)
    except FileNotFoundError:
        return jsonify({"error": "File not found"}), 404
    except Exception as e:
        return jsonify({"error": str(e)}), 500
    
    # Assuming we add the color successfully
    response = {"message": "Color removed successfully", "colors": orig_colors['colors']}
    return jsonify(response), 200

@app.route('/sndeffects', methods=['GET'])
async def handle_get_all_sound_effects():
    """Handles GET sound effects"""
    file_path = "json/sound-effects.json"

    try:
        effects = get_json_data(file_path)
        return jsonify(effects)
    except FileNotFoundError:
        return jsonify({"error": "File not found"}), 404
    except json.JSONDecodeError:
        return jsonify({"error": "Error decoding JSON"}), 500
    
@app.route('/sndeffects', methods=['PUT'])
async def handle_set_sound_effect():
    try:
        data = await request.get_json()
        if len(data) < 1:
            return jsonify({'error', 'Invalid JSON'}), 400
        effect = list(data.keys())[0]

        sound_capture.effect_name = effect

        return jsonify({'success': 'effect successfully set', 'effect_name': effect}), 200
    except Exception as e:
        return jsonify({'error': str(e)}), 500

def get_json_data(file_path):
    try:
        with open(file_path, 'r') as f:
            data = json.load(f)  # Load JSON data from file
        return data
    except FileNotFoundError:
        print(f"File not found: {file_path}")
        raise
    except json.JSONDecodeError:
        print(f"Error decoding JSON from file: {file_path}")
        raise
    except Exception as e:
        print(f"Unexpected error when reading {file_path}: {e}")
        raise


def write_json_data(file_path, data):
    with open(file_path, 'w') as f:
        json.dump(data, f, indent=2)

def update_all_vars():
    capt_data = get_json_data("json/capt_values.json")
    change_dict_vars(capt_data, sc.sc_settings)

    led_data = get_json_data("json/led_values.json")
    change_dict_vars(led_data, lf.led_values)

    

async def main():
    # Update LEDs at startup
    await lf.update_leds()

    update_all_vars()
    
    # Start the Quart app
    await app.run_task(host='0.0.0.0', port=80, debug=True)

if __name__ == '__main__':
    asyncio.run(main())
