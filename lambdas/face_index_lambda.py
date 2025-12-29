import json
import boto3
import logging

# Configure logging
logger = logging.getLogger()
logger.setLevel(logging.INFO)

def lambda_handler(event, context):
    s3 = boto3.client('s3')
    rekognition = boto3.client('rekognition')
    
    # Log the received event
    logger.info(f"Received event: {json.dumps(event)}")
    
    # Get the S3 bucket and object key from the event
    bucket = event['Records'][0]['s3']['bucket']['name']
    key = event['Records'][0]['s3']['object']['key']
    
    # Extract the external image ID from the key (assuming the format is student1.jpg)
    external_image_id = key.split('.')[0]
    logger.info(f"Extracted External Image ID: {external_image_id}")

    # Call Rekognition to index faces
    try:
        response = rekognition.index_faces(
            CollectionId='students',  # Replace with your collection ID
            Image={
                'S3Object': {
                    'Bucket': bucket,
                    'Name': key
                }
            },
            ExternalImageId=external_image_id,
            DetectionAttributes=['ALL']
        )
        
        logger.info(f"Rekognition response: {json.dumps(response)}")

        return {
            'statusCode': 200,
            'body': json.dumps('Face indexing completed')
        }
    except Exception as e:
        # Log the error message with detailed error response
        logger.error(f"Error indexing faces: {str(e)}")
        if hasattr(e, 'response'):
            logger.error(f"Error details: {json.dumps(e.response)}")  # Log detailed error response
        
        return {
            'statusCode': 500,
            'body': json.dumps('Face indexing failed')
        }