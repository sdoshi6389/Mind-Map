from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build

# If modifying scopes, delete token.json
SCOPES = ['https://www.googleapis.com/auth/contacts.readonly']

def get_google_contacts():
    creds = None
    if creds is None:
        flow = InstalledAppFlow.from_client_secrets_file('credentials.json', SCOPES)
        creds = flow.run_local_server(port=0)

    service = build('people', 'v1', credentials=creds)

    results = service.people().connections().list(
        resourceName='people/me',
        pageSize=2000,
        personFields='names,emailAddresses,phoneNumbers'
    ).execute()

    connections = results.get('connections', [])
    contacts = []

    for person in connections:
        names = person.get('names', [])
        emails = person.get('emailAddresses', [])
        phones = person.get('phoneNumbers', [])

        name = names[0]['displayName'] if names else 'Unnamed'
        email = emails[0]['value'] if emails else 'No Email'
        phone = phones[0]['value'] if phones else 'No Phone'

        contacts.append((name, email, phone))

    return contacts

def save_contacts_to_txt(contacts, filename="contacts.txt"):
    with open(filename, "w", encoding="utf-8") as f:
        for name, email, phone in contacts:
            f.write(f"{name} | {email} | {phone}\n")

if __name__ == '__main__':
    contacts = get_google_contacts()
    print(f"Pulled {len(contacts)} contacts.")
    save_contacts_to_txt(contacts)
    print("Contacts saved to contacts.txt")
