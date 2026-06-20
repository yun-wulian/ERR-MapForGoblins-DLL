using System.Text;
using Org.BouncyCastle.Crypto;
using Org.BouncyCastle.Crypto.Engines;
using Org.BouncyCastle.OpenSsl;
using SoulsFormats;

static string NormalizePath(string path)
{
    string normalized = path.Replace('\\', '/').Trim();
    if (!normalized.StartsWith('/'))
        normalized = '/' + normalized;
    return normalized.ToLowerInvariant();
}

static ulong ComputeHash(string path)
{
    string hashable = NormalizePath(path);
    ulong hash = 0;
    foreach (char c in hashable)
        hash = hash * 0x85ul + c;
    return hash;
}

static Dictionary<ulong, string> LoadDictionary(string path)
{
    if (!File.Exists(path))
        throw new FileNotFoundException("Archive dictionary not found", path);

    var hashes = new Dictionary<ulong, string>();
    foreach (string raw in File.ReadLines(path))
    {
        string line = raw.Trim();
        if (line.Length == 0 || line.StartsWith('#'))
            continue;
        hashes[ComputeHash(line)] = line;
    }
    return hashes;
}

static bool IsWanted(string normalized)
{
    return (normalized.StartsWith("/map/mapstudio/", StringComparison.Ordinal) && normalized.EndsWith(".msb.dcx", StringComparison.Ordinal))
        || (normalized.StartsWith("/event/", StringComparison.Ordinal) && normalized.EndsWith(".emevd.dcx", StringComparison.Ordinal))
        || (normalized.StartsWith("/msg/", StringComparison.Ordinal) && normalized.EndsWith(".msgbnd.dcx", StringComparison.Ordinal));
}

static string DestinationPath(string outDir, string normalized)
{
    string trimmed = normalized.TrimStart('/');
    string[] parts = trimmed.Split('/', StringSplitOptions.RemoveEmptyEntries);
    if (parts.Any(p => p == ".." || p.Contains(':')))
        throw new InvalidOperationException($"Refusing suspicious archive path: {normalized}");

    if (parts.Length >= 2 && parts[0].Equals("map", StringComparison.OrdinalIgnoreCase) && parts[1].Equals("mapstudio", StringComparison.OrdinalIgnoreCase))
        parts[1] = "MapStudio";

    return Path.Combine(new[] { outDir }.Concat(parts).ToArray());
}

static void CopyIfExists(string src, string dst)
{
    if (!File.Exists(src))
        throw new FileNotFoundException(src);
    Directory.CreateDirectory(Path.GetDirectoryName(dst)!);
    File.Copy(src, dst, overwrite: true);
}

static MemoryStream DecryptRsa(string filePath, string key)
{
    AsymmetricKeyParameter keyParameter = (AsymmetricKeyParameter)new PemReader(new StringReader(key)).ReadObject();
    var engine = new RsaEngine();
    engine.Init(false, keyParameter);

    var output = new MemoryStream();
    using var input = File.OpenRead(filePath);
    int inputBlockSize = engine.GetInputBlockSize();
    int outputBlockSize = engine.GetOutputBlockSize();
    byte[] inputBlock = new byte[inputBlockSize];

    int read;
    while ((read = input.Read(inputBlock, 0, inputBlock.Length)) > 0)
    {
        byte[] outputBlock = engine.ProcessBlock(inputBlock, 0, read);
        int requiredPadding = outputBlockSize - outputBlock.Length;
        if (requiredPadding > 0)
        {
            byte[] padded = new byte[outputBlockSize];
            outputBlock.CopyTo(padded, requiredPadding);
            outputBlock = padded;
        }
        output.Write(outputBlock, 0, outputBlock.Length);
    }

    output.Position = 0;
    return output;
}

static BHD5 ReadBhd(string bhdPath, string key)
{
    byte[] magic = new byte[4];
    using (FileStream fs = File.OpenRead(bhdPath))
    {
        if (fs.Read(magic, 0, magic.Length) != magic.Length)
            throw new InvalidDataException($"Unable to read BHD magic: {bhdPath}");
    }

    if (Encoding.ASCII.GetString(magic) == "BHD5")
        return BHD5.Read(File.ReadAllBytes(bhdPath), BHD5.Game.EldenRing);

    using MemoryStream decrypted = DecryptRsa(bhdPath, key);
    return BHD5.Read(decrypted.ToArray(), BHD5.Game.EldenRing);
}

static int ExtractArchive(string gameDir, string outDir, string archive, string key, IReadOnlyDictionary<ulong, string> names)
{
    string bhdPath = Path.Combine(gameDir, archive + ".bhd");
    string bdtPath = Path.Combine(gameDir, archive + ".bdt");
    if (!File.Exists(bhdPath) || !File.Exists(bdtPath))
    {
        Console.WriteLine($"Skipping {archive}: BHD/BDT not found");
        return 0;
    }

    Console.WriteLine($"Reading {archive}...");
    BHD5 bhd = ReadBhd(bhdPath, key);
    int seen = bhd.Buckets.Sum(b => b.Count);
    int extracted = 0;

    using FileStream bdt = File.OpenRead(bdtPath);
    foreach (BHD5.Bucket bucket in bhd.Buckets)
    {
        foreach (BHD5.FileHeader header in bucket)
        {
            if (!names.TryGetValue(header.FileNameHash, out string? archivePath))
                continue;

            string normalized = NormalizePath(archivePath);
            if (!IsWanted(normalized))
                continue;

            string dst = DestinationPath(outDir, normalized);
            Directory.CreateDirectory(Path.GetDirectoryName(dst)!);
            byte[] bytes = header.ReadFile(bdt);
            File.WriteAllBytes(dst, bytes);
            extracted++;
            if (extracted % 250 == 0)
                Console.WriteLine($"  {archive}: extracted {extracted}...");
        }
    }

    Console.WriteLine($"  {archive}: extracted {extracted} / {seen} entries");
    return extracted;
}

if (args.Length < 3)
{
    Console.Error.WriteLine("usage: ExtractVanillaLoose <packed-game-dir> <out-loose-dir> <dictionary-path>");
    return 2;
}

string gameDir = Path.GetFullPath(args[0]);
string outDir = Path.GetFullPath(args[1]);
string dictionaryPath = Path.GetFullPath(args[2]);

Directory.CreateDirectory(outDir);
CopyIfExists(Path.Combine(gameDir, "regulation.bin"), Path.Combine(outDir, "regulation.bin"));
CopyIfExists(Path.Combine(gameDir, "oo2core_6_win64.dll"), Path.Combine(outDir, "oo2core_6_win64.dll"));

Console.WriteLine($"Dictionary: {dictionaryPath}");
Dictionary<ulong, string> names = LoadDictionary(dictionaryPath);
Console.WriteLine($"Loaded {names.Count} dictionary entries");

var keys = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
{
    ["Data0"] = @"-----BEGIN RSA PUBLIC KEY-----
MIIBCwKCAQEA9Rju2whruXDVQZpfylVEPeNxm7XgMHcDyaaRUIpXQE0qEo+6Y36L
P0xpFvL0H0kKxHwpuISsdgrnMHJ/yj4S61MWzhO8y4BQbw/zJehhDSRCecFJmFBz
3I2JC5FCjoK+82xd9xM5XXdfsdBzRiSghuIHL4qk2WZ/0f/nK5VygeWXn/oLeYBL
jX1S8wSSASza64JXjt0bP/i6mpV2SLZqKRxo7x2bIQrR1yHNekSF2jBhZIgcbtMB
xjCywn+7p954wjcfjxB5VWaZ4hGbKhi1bhYPccht4XnGhcUTWO3NmJWslwccjQ4k
sutLq3uRjLMM0IeTkQO6Pv8/R7UNFtdCWwIERzH8IQ==
-----END RSA PUBLIC KEY-----",

    ["Data1"] = @"-----BEGIN RSA PUBLIC KEY-----
MIIBCwKCAQEAxaBCHQJrtLJiJNdG9nq3deA9sY4YCZ4dbTOHO+v+YgWRMcE6iK6o
ZIJq+nBMUNBbGPmbRrEjkkH9M7LAypAFOPKC6wMHzqIMBsUMuYffulBuOqtEBD11
CAwfx37rjwJ+/1tnEqtJjYkrK9yyrIN6Y+jy4ftymQtjk83+L89pvMMmkNeZaPON
4O9q5M9PnFoKvK8eY45ZV/Jyk+Pe+xc6+e4h4cx8ML5U2kMM3VDAJush4z/05hS3
/bC4B6K9+7dPwgqZgKx1J7DBtLdHSAgwRPpijPeOjKcAa2BDaNp9Cfon70oC+ZCB
+HkQ7FjJcF7KaHsH5oHvuI7EZAl2XTsLEQIENa/2JQ==
-----END RSA PUBLIC KEY-----",

    ["Data2"] = @"-----BEGIN RSA PUBLIC KEY-----
MIIBDAKCAQEA0iDVVQ230RgrkIHJNDgxE7I/2AaH6Li1Eu9mtpfrrfhfoK2e7y4O
WU+lj7AGI4GIgkWpPw8JHaV970Cr6+sTG4Tr5eMQPxrCIH7BJAPCloypxcs2BNfT
GXzm6veUfrGzLIDp7wy24lIA8r9ZwUvpKlN28kxBDGeCbGCkYeSVNuF+R9rN4OAM
RYh0r1Q950xc2qSNloNsjpDoSKoYN0T7u5rnMn/4mtclnWPVRWU940zr1rymv4Jc
3umNf6cT1XqrS1gSaK1JWZfsSeD6Dwk3uvquvfY6YlGRygIlVEMAvKrDRMHylsLt
qqhYkZNXMdy0NXopf1rEHKy9poaHEmJldwIFAP////8=
-----END RSA PUBLIC KEY-----",

    ["Data3"] = @"-----BEGIN RSA PUBLIC KEY-----
MIIBCwKCAQEAvRRNBnVq3WknCNHrJRelcEA2v/OzKlQkxZw1yKll0Y2Kn6G9ts94
SfgZYbdFCnIXy5NEuyHRKrxXz5vurjhrcuoYAI2ZUhXPXZJdgHywac/i3S/IY0V/
eDbqepyJWHpP6I565ySqlol1p/BScVjbEsVyvZGtWIXLPDbx4EYFKA5B52uK6Gdz
4qcyVFtVEhNoMvg+EoWnyLD7EUzuB2Khl46CuNictyWrLlIHgpKJr1QD8a0ld0PD
PHDZn03q6QDvZd23UW2d9J+/HeBt52j08+qoBXPwhndZsmPMWngQDaik6FM7EVRQ
etKPi6h5uprVmMAS5wR/jQIVTMpTj/zJdwIEXszeQw==
-----END RSA PUBLIC KEY-----",

    ["DLC"] = @"-----BEGIN RSA PUBLIC KEY-----
MIIBCwKCAQEAmYJ/5GJU4boJSvZ81BFOHYTGdBWPHnWYly3yWo01BYjGRnz8NTkz
DHUxsbjIgtG5XqsQfZstZILQ97hgSI5AaAoCGrT8sn0PeXg2i0mKwL21gRjRUdvP
Dp1Y+7hgrGwuTkjycqqsQ/qILm4NvJHvGRd7xLOJ9rs2zwYhceRVrq9XU2AXbdY4
pdCQ3+HuoaFiJ0dW0ly5qdEXjbSv2QEYe36nWCtsd6hEY9LjbBX8D1fK3D2c6C0g
NdHJGH2iEONUN6DMK9t0v2JBnwCOZQ7W+Gt7SpNNrkx8xKEM8gH9na10g9ne11Mi
O1FnLm8i4zOxVdPHQBKICkKcGS1o3C2dfwIEXw/f3w==
-----END RSA PUBLIC KEY-----",
};

int total = 0;
foreach (var archive in new[] { "Data0", "Data1", "Data2", "Data3", "DLC" })
    total += ExtractArchive(gameDir, outDir, archive, keys[archive], names);

Console.WriteLine($"Done. Extracted {total} filtered files to {outDir}");
Console.WriteLine($"  map MSBs: {Directory.EnumerateFiles(Path.Combine(outDir, "map", "MapStudio"), "*.msb.dcx", SearchOption.TopDirectoryOnly).Count()}");
Console.WriteLine($"  events:   {Directory.EnumerateFiles(Path.Combine(outDir, "event"), "*.emevd.dcx", SearchOption.TopDirectoryOnly).Count()}");
Console.WriteLine($"  msgbnds:  {Directory.EnumerateFiles(Path.Combine(outDir, "msg"), "*.msgbnd.dcx", SearchOption.AllDirectories).Count()}");
return total > 0 ? 0 : 1;
