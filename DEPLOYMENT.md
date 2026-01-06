# Deployment Guide

This guide explains how to deploy the Air Travel Database application to Render or other cloud platforms.

## Prerequisites

- A GitHub account
- A Render account (free tier available at https://render.com)
- Git installed on your local machine

## Option 1: Deploy to Render (Recommended)

### Step 1: Push to GitHub

1. Initialize git repository (if not already done):
```bash
git init
git add .
git commit -m "Initial commit"
```

2. Create a new repository on GitHub and push:
```bash
git remote add origin https://github.com/YOUR_USERNAME/YOUR_REPO_NAME.git
git branch -M main
git push -u origin main
```

### Step 2: Deploy on Render

1. Go to https://render.com and sign up/login
2. Click "New +" → "Web Service"
3. Connect your GitHub account and select your repository
4. Configure the service:
   - **Name**: air-travel-db (or any name you prefer)
   - **Environment**: Docker
   - **Region**: Choose closest to you
   - **Branch**: main
   - **Root Directory**: (leave empty)
   - **Dockerfile Path**: `./Dockerfile`
   - **Docker Context**: `.`
   - **Plan**: Free (or paid if you prefer)
   - **Auto-Deploy**: Yes (optional)

5. Click "Create Web Service"

6. Render will automatically:
   - Build the Docker image
   - Deploy the application
   - Provide you with a public URL (e.g., `https://air-travel-db.onrender.com`)

### Step 3: Access Your Application

Once deployment is complete, Render will provide you with a public URL. The application will be accessible at that URL.

**Note**: On the free tier, Render may spin down your service after 15 minutes of inactivity. The first request after spin-down may take 30-60 seconds to respond.

## Option 2: Deploy with Docker Locally

### Build the Docker image:
```bash
docker build -t air-travel-db .
```

### Run the container:
```bash
docker run -p 8080:8080 air-travel-db
```

The application will be available at `http://localhost:8080`

### Run with custom port:
```bash
docker run -p 3000:8080 -e PORT=8080 air-travel-db
```

## Option 3: Deploy to Other Platforms

### Fly.io

1. Install flyctl: https://fly.io/docs/getting-started/installing-flyctl/
2. Login: `fly auth login`
3. Launch: `fly launch`
4. Deploy: `fly deploy`

### Railway

1. Install Railway CLI: `npm i -g @railway/cli`
2. Login: `railway login`
3. Initialize: `railway init`
4. Deploy: `railway up`

### Heroku

1. Install Heroku CLI
2. Login: `heroku login`
3. Create app: `heroku create your-app-name`
4. Deploy: `git push heroku main`

## Environment Variables

- `PORT`: The port the server should listen on (default: 8080)
  - Render automatically sets this, but you can override it if needed

## Troubleshooting

### Build fails on Render

- Check that all `.dat` files are committed to git
- Ensure Dockerfile is in the root directory
- Check build logs in Render dashboard

### Application doesn't start

- Verify the PORT environment variable is set correctly
- Check application logs in Render dashboard
- Ensure the binary is executable

### Slow first request (Render free tier)

- This is normal on the free tier due to spin-down
- Consider upgrading to a paid plan for always-on service

## Updating Your Deployment

Simply push changes to your GitHub repository, and Render will automatically rebuild and redeploy:

```bash
git add .
git commit -m "Update application"
git push
```

