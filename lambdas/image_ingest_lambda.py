import boto3
import base64
import json
from datetime import datetime  # Importing datetime for filename generation

# Initialize S3 client and set bucket name
s3 = boto3.client('s3')
bucket_name = 'esiot-project-face'

def lambda_handler(event, context):
    # Log the event received from IoT rule
    print(f"Received event: {json.dumps(event)}")  # Log the entire event for debugging

    # Directly access and decode the Base64-encoded payload
    try:
        # Decode the Base64 image data directly from the payload
        image_data = base64.b64decode(event['payload'])
        print("Decoded image data successfully.")
    except Exception as e:
        print(f"Error decoding image data: {e}")
        return {
            'statusCode': 500,
            'body': "Error decoding image data."
        }

    # Generate a unique filename based on the current date and time
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")  # Format: YYYYMMDD_HHMMSS
    filename = f'image_{timestamp}.jpg'
    
    try:
        # Upload the decoded image to S3
        s3.put_object(Bucket=bucket_name, Key=filename, Body=image_data, ContentType='image/jpeg')
        print(f"Image {filename} uploaded successfully to S3.")
    except Exception as e:
        print(f"Failed to upload image {filename} to S3: {e}")
        return {
            'statusCode': 500,
            'body': f"Failed to upload image {filename}."
        }

    return {
        'statusCode': 200,
        'body': f"Image {filename} uploaded successfully!"
    }