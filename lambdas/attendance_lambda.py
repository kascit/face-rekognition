import json
import boto3
import os
import logging
from datetime import datetime, timedelta

# Configure logging
logger = logging.getLogger()
logger.setLevel(logging.INFO)

def lambda_handler(event, context):
    s3 = boto3.client('s3')
    rekognition = boto3.client('rekognition')
    dynamodb = boto3.client('dynamodb')
    
    # Access environment variables
    collection_id = os.environ['REKOGNITION_COLLECTION_ID']
    table_name = os.environ['DYNAMODB']
    
    try:
        # Get the S3 bucket and object key from the event
        bucket = event['Records'][0]['s3']['bucket']['name']
        key = event['Records'][0]['s3']['object']['key']
        logger.info(f"Received S3 event for bucket: {bucket}, key: {key}")

        # Call Rekognition to search for faces in the collection
        response = rekognition.search_faces_by_image(
            CollectionId=collection_id,
            Image={
                'S3Object': {
                    'Bucket': bucket,
                    'Name': key
                }
            },
            MaxFaces=1,
            FaceMatchThreshold=95
        )
        
        logger.info(f"Rekognition response: {response}")
        
        # Process the response and update DynamoDB
        if response['FaceMatches']:
            face_match = response['FaceMatches'][0]
            external_image_id = face_match['Face']['ExternalImageId']  # Roll number
            timestamp = event['Records'][0]['eventTime']
            
            # Convert timestamp to acceptable date format
            timestamp_dt = datetime.strptime(timestamp, '%Y-%m-%dT%H:%M:%S.%fZ')
            date_today = timestamp_dt.strftime('%Y-%m-%d')

            # Adjust for IST
            ist_dt = timestamp_dt + timedelta(hours=5, minutes=30)
            ist_time = ist_dt.strftime('%Y-%m-%d %H:%M:%S')

            # Log the IST time of picture capture
            logger.info(f"Picture taken at (IST): {ist_time}")

            # Determine the attendance period
            period = determine_period(ist_dt)
            in_time_key = f"in_time{period}"
            out_time_key = f"out_time{period}"
            attendance_status_key = f"period_{period}"

            # Get current record for the day
            response = dynamodb.get_item(
                TableName=table_name,
                Key={
                    'RollNumber': {'S': external_image_id},
                    'Date': {'S': date_today}
                }
            )

            logger.info(f"DynamoDB response for {external_image_id} on {date_today}: {response}")

            # Initialize in_time, out_time, and attendance_status for the period
            item = response.get('Item', {})
            in_time = item.get(in_time_key, {}).get('S')
            out_time = item.get(out_time_key, {}).get('S')

            logger.info(f"Current in_time: {in_time}, out_time: {out_time} for {external_image_id} on {date_today}")

            # Update in_time if it doesn't exist
            if not in_time:
                dynamodb.update_item(
                    TableName=table_name,
                    Key={
                        'RollNumber': {'S': external_image_id},
                        'Date': {'S': date_today}
                    },
                    UpdateExpression=f"SET {in_time_key} = :in_time",
                    ExpressionAttributeValues={
                        ':in_time': {'S': ist_time}
                    }
                )
                logger.info(f"{in_time_key} set for {external_image_id} on {date_today}.")
            else:
                # Always update out_time and check time difference
                dynamodb.update_item(
                    TableName=table_name,
                    Key={
                        'RollNumber': {'S': external_image_id},
                        'Date': {'S': date_today}
                    },
                    UpdateExpression=f"SET {out_time_key} = :out_time",
                    ExpressionAttributeValues={
                        ':out_time': {'S': ist_time}
                    }
                )
                logger.info(f"{out_time_key} updated for {external_image_id} on {date_today}.")

                # Check if time difference between in_time and out_time is >= 30 minutes
                in_time_dt = datetime.strptime(in_time, '%Y-%m-%d %H:%M:%S')
                time_difference = ist_dt - in_time_dt
                logger.info(f"Time difference for {external_image_id} on {date_today}: {time_difference}")

                if time_difference >= timedelta(minutes=30):
                    dynamodb.update_item(
                        TableName=table_name,
                        Key={
                            'RollNumber': {'S': external_image_id},
                            'Date': {'S': date_today}
                        },
                        UpdateExpression=f"SET {attendance_status_key} = :status",
                        ExpressionAttributeValues={
                            ':status': {'S': 'Present'}
                        }
                    )
                    logger.info(f"Attendance marked for {external_image_id} on {date_today} for period {period}.")
                else:
                    logger.info(f"Time difference less than 30 minutes for {external_image_id} on {date_today} for period {period}. No attendance marked.")
        
        else:
            logger.info("No face matches found.")
        
    except Exception as e:
        logger.error(f"Error processing S3 event: {e}")
        return {
            'statusCode': 500,
            'body': json.dumps('Error processing face recognition')
        }
    
    return {
        'statusCode': 200,
        'body': json.dumps('Face recognition and attendance update completed')
    }

def determine_period(timestamp_dt):
    """
    Function to determine the period based on the time.
    Periods are defined as hourly from 9 AM to 4 PM IST.
    """
    hour = timestamp_dt.hour

    if 9 <= hour < 10:
        return 1
    elif 10 <= hour < 11:
        return 2
    elif 11 <= hour < 12:
        return 3
    elif 12 <= hour < 13:
        return 4
    elif 13 <= hour < 14:
        return 5
    elif 14 <= hour < 15:
        return 6
    elif 15 <= hour < 16:
        return 7
    else:
        return 0  # Outside of attendance periods