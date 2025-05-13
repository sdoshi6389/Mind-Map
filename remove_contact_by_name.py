import sqlite3

def remove_contact_by_name(name):
    try:
        conn = sqlite3.connect("my_database.db")
        cur = conn.cursor()

        # Get contact ID by name
        cur.execute("SELECT id FROM contacts WHERE name = ?", (name,))
        result = cur.fetchone()

        if not result:
            print(f"[INFO] No contact found with name: {name}")
            return

        contact_id = result[0]

        # Delete from all related tables
        cur.execute("DELETE FROM background WHERE contact_id = ?", (contact_id,))
        cur.execute("DELETE FROM profile WHERE contact_id = ?", (contact_id,))
        cur.execute("DELETE FROM relationships WHERE contact_id = ?", (contact_id,))
        cur.execute("DELETE FROM employment WHERE contact_id = ?", (contact_id,))
        cur.execute("DELETE FROM contacts WHERE id = ?", (contact_id,))

        conn.commit()
        conn.close()
        print(f"[SUCCESS] Contact '{name}' and related data removed.")
    except Exception as e:
        print("[ERROR]", e)

# Example usage:
if __name__ == "__main__":
    name = input("Enter name to remove: ").strip()
    remove_contact_by_name(name)
