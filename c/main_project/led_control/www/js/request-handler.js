  async function sendLedSettingsPost(data) {
    try {
      const response = await fetch ('/led-settings', {
        method: 'POST',
        headers: {
          'Content-type': 'application/json',
        },
        body: JSON.stringify(data),
      });

      
      if (!response.ok) {
        message_pop_up(TYPE.ERROR, "Error capturing screen", response.status);
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
    } catch (error) {
      message_pop_up(TYPE.ERROR, "Error capturing screen", error);
      console.log("Error: ", error);
    }
  }

  async function sendLedSettingsGet() {
    try {
      const response = await fetch("/led-settings", {
        method: "GET",
        headers: {
          "Content-Type": "application/json"
        },
      });

      if(!response.ok) {
        message_pop_up(TYPE.ERROR, "Error getting settings:\n", response.status);
        throw new Error(`HTTP error! Status: ${response.status}`);
      }

      dataPart = await response.json();
      return dataPart.data;
    } catch (error) {
      message_pop_up(TYPE.ERROR, "Error:", error);
      console.error("Error:", error)
    }
  }
  
  async function requestCapture(isCapture) {
    try {
      const response = await fetch('/led-settings', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(data),
    });

    if (!response.ok) {
      message_pop_up(TYPE.ERROR, "Error capturing screen", response.status);
      throw new Error(`HTTP error! Status: ${response.status}`);
    }

    return await response.json()
    } catch (error) {
      message_pop_up(TYPE.ERROR, "Error capturing screen", error);
      console.log("Error: ", error);
    }
  }

  async function changeLedColor(colorString) {
    try {
      hexColor = colorString.slice(1);

      const response = await fetch(`/led?fx=0&capt=0&srea=0&col=${hexColor}`, {
        method : 'GET', 
        headers: {
          'Content-type' : 'application/json',
        },
      });

      if (!response.ok) {
        message_pop_up(TYPE.ERROR, "Error changing color:\n", response.status);
        throw new Error(`HTTP error! Status: ${response.status}`);
      }

      return await response.json();
    } catch (error) {
      message_pop_up(TYPE.ERROR, "Error:", error);
      console.error("Error:", error)
    }

  }

  async function getCurrColor() {
    try {
        const response = await fetch("/led/col", {
            method: 'GET',
            headers: {
                'Content-Type': 'application/json',
            }
        });

        if (!response.ok) {
          message_pop_up(TYPE.ERROR, "Error loading current color:\n", response.status);
          throw new Error(`HTTP error! Status: ${response.status}`);
        }

        return await response.json();
    } catch (error) {
        console.error("Error:", error);
    }
  }

  async function getUserColors() {
    try {
      const response = await fetch("/led/user/colors", {
        method: 'GET',
        headers: {
          'Content-Type': 'application/json',
        }
      });

      if (!response.ok) {
        message_pop_up(TYPE.ERROR, "Error loading user colors:\n", response.status);
        throw new Error(`HTTP error! Status: ${response.status}`);
      }

      return await response.json();
    } catch (error) {
      console.error("Error:", error);
      message_pop_up(TYPE.ERROR, "Error loading user colors:\n", error);
    }
  }

  async function requestGetLedSettings() {
      try {
        const response = await fetch(`/led`, {
          method : 'GET', 
          headers: {
            'Content-type' : 'application/json',
          },
        });
    
        if (!response.ok) {
          message_pop_up(TYPE.ERROR, "Error getting LED values:\n", response.status);
          throw new Error(`HTTP error! Status: ${response.status}`);
        }
    
        return await response.json();
      } catch (error) {
        console.error("Error:", error);
        message_pop_up(TYPE.ERROR, "Error getting LED values", error.message);
      }
  }

  async function requestAddUserColor(color) {
    try {
        const response = await fetch("/led/user/colors/add", {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ color }) // Send the color data in the request body
        });
        
        if (!response.ok) {
            const errorMessage = `Error adding color: ${response.status}`;
            message_pop_up(TYPE.ERROR, errorMessage);
            throw new Error(errorMessage);
        }

        return await response.json();
    } catch (error) {
        console.error("Error:", error);
        message_pop_up(TYPE.ERROR, "Error adding color:", error.message);
    }
}

async function requestRemoveUserColor(color) {
  try {
      const response = await fetch("/led/user/colors/remove", {
          method: 'POST',
          headers: {
              'Content-Type': 'application/json',
          },
          body: JSON.stringify({ color }) // Send the color data in the request body
      });
      
      if (!response.ok) {
          const errorMessage = `Error removing color: ${response.status}`;
          message_pop_up(TYPE.ERROR, errorMessage);
          throw new Error(errorMessage);
      }

      return await response.json();
  } catch (error) {
      console.error("Error:", error);
      message_pop_up(TYPE.ERROR, "Error removing color:", error.message);
  }
}

async function requestChangeBrightness(brightness) {
  try {
    const response = await fetch(`/led?bri=${brightness}`, {
      method : 'GET', 
      headers: {
        'Content-type' : 'application/json',
      },
    });

    if (!response.ok) {
      message_pop_up(TYPE.ERROR, "Error changing brightness:\n", response.status);
      throw new Error(`HTTP error! Status: ${response.status}`);
    }

    return await response.json();
  } catch (error) {
    console.error("Error:", error);
    message_pop_up(TYPE.ERROR, "Error changing brightness:", error.message);
  }
}

async function requestChangeLedCount(count) {
    try {
      const response = await fetch(`/led?cnt=${count}`, {
        method : 'GET', 
        headers: {
          'Content-type' : 'application/json',
        },
      });
  
      if (!response.ok) {
        message_pop_up(TYPE.ERROR, "Error changing count:\n", response.status);
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
  
      return await response.json();
    } catch (error) {
      console.error("Error:", error);
      message_pop_up(TYPE.ERROR, "Error changing count:", error.message);
    }
}

async function requestChangeSoundReact(value) {
  try {
    const response = await fetch(`/led?srea=${value}`, {
      method: 'GET',
      headers: {
        'Content-Type' : 'application/json',
      }
    })

    if(!response.ok) {
      message_pop_up(TYPE.ERROR, "Error updating sound react:", response.status);
      throw new Error(`HTTP error! Status: ${response.status}`);
    }

    return await response.json()
  } catch (error) {
    console.error("Error:", error);
    message_pop_up(TYPE.ERROR, "Error updating sound react:", error.message);
  }
}

async function requestChangeCapt(params) {
  try {
    const response = await fetch(`/scrncapt?${params}`, {
      method:'GET',
      headers: {
        'Content-type' : 'application/json',
      },
    });

    if (!response.ok) {
      message_pop_up(TYPE.ERROR, "Error changing numbers:\n", response.status);
      throw new Error(`HTTP error! Status: ${response.status}`);
    }

    return await response.json();
  }catch (error) {
    console.error("Error:", error);
    message_pop_up(TYPE.ERROR, "Error changing numbers:", error.message);
  }
}

async function requestGetCaptSettings() {
  try {
    const response = await fetch(`/scrncapt`, {
      method:'GET',
      headers: {
        'Content-type' : 'application/json',
      },
    });

    if (!response.ok) {
      message_pop_up(TYPE.ERROR, "Error getting capture settings:\n", response.status);
      throw new Error(`HTTP error! Status: ${response.status}`);
    }

    return await response.json();
  }catch (error) {
    console.error("Error:", error);
    message_pop_up(TYPE.ERROR, "Error getting capture settings", error.message);
  }
} 

async function requestGetSoundEffects() {
  try {
    const response = await fetch('/sndeffects', {
      method: 'GET',
      headers: {
        'Content-type' : 'application/json'
      }
    });

    if(!response.ok) {
      message_pop_up(TYPE.ERROR, "Error getting the sound effects")
      throw new Error(`HTTP Error! Status: ${response.status}`);
    } 

    return await response.json();
  } catch (error) {
    console.error("Error: ", error)
  }
}

async function requestSetSoundEffect(effectName, effectPath) {
  try {
    const response = await fetch('/sndeffects', {
      method: 'PUT',
      headers: {
        'Content-type': 'application/json'
      },
      body: JSON.stringify({[effectName]: effectPath})
    });
    if(!response.ok) {
      message_pop_up(TYPE.ERROR, "Error getting the sound effects")
      throw new Error(`HTTP Error! Status: ${response.status}`);
    }

    return await response.json();
  } catch (error) {
    console.error("Error: ", error.message)
  }
}