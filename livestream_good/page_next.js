"use client"
import { useEffect, useRef } from 'react';

const Home = () => {
  // Specify the type of the ref to be HTMLCanvasElement or null
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const wsRef = useRef<WebSocket | null>(null);

  useEffect(() => {
    // Connect to the WebSocket server
    const ws = new WebSocket('ws://192.168.0.17:3001'); // Use your server URL if not localhost
    wsRef.current = ws;

    // Get the canvas and its context
    const canvas = canvasRef.current;
    
    // Check if canvas is not null before accessing its context
    if (canvas) {
      const context = canvas.getContext('2d');

      // When WebSocket connection is established
      ws.onopen = () => {
        console.log('WebSocket connection established');
      };

      // When WebSocket receives a message (frame)
      ws.onmessage = (event) => {
        if (event.data instanceof Blob && context) {
          const imageBlob = event.data;
          const img = new Image();

          img.onload = () => {
            context.clearRect(0, 0, canvas.width, canvas.height); // Clear previous frame
            context.drawImage(img, 0, 0, canvas.width, canvas.height); // Draw the new image
          };

          img.src = URL.createObjectURL(imageBlob); // Convert Blob to Image
        } else {
          console.warn('Received non-binary data from WebSocket');
        }
      };

      // WebSocket error handling
      ws.onerror = (error) => {
        console.error('WebSocket error:', error);
      };

      // WebSocket connection close
      ws.onclose = () => {
        console.log('WebSocket connection closed');
      };
    }

    // Clean up WebSocket connection when component unmounts
    return () => {
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, []);

  return (
    <div style={{ textAlign: 'center' }}>
      <h1>Live Video Stream from ESP32</h1>
      <canvas ref={canvasRef} width={640} height={480} style={{ border: '1px solid black' }} />
    </div>
  );
};

export default Home;